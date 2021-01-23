/*
 Copyright 2011 G. Andrew Stone
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Arduino.h"
#include "lin.h"

 //void p(char *fmt, ... );

LinScheduleEntry::LinScheduleEntry()
{
  // Just clear it out
  skewChildren.Left = 0; skewChildren.Right = 0;
  trigger = 0;
  callback = 0;
  addr = 0;
  len  = 0;
}

Lin::Lin(LIN_SERIAL& ser,uint8_t TxPin): serial(ser), txPin(TxPin)
{
}

void Lin::loop()
{
  LinScheduleEntry& e = scheduler.front();
  
  if ((&e)&&(e.trigger < millis()))
    {
      // remove this frame from the top of the heap.
      scheduler.pop();
      uint8_t proto = (e.flags&Lin2Frame) ? 2:1; 
      // Do the correct LIN operation
      if (e.flags&LinReadFrame) recv(e.addr,e.data,e.len,proto);
      else                      send(e.addr,e.data,e.len,proto);

      // If there is a callback function, call it.
      if (e.callback)
        {
        uint16_t reschedule = e.callback(&e);
        if (reschedule) add(e,reschedule);  // Put the frame back on if the caller wants to repeat.
        }
    }  
}

void Lin::begin(int speed)
{
  serialSpd = speed;
  serial.begin(serialSpd);
  serialOn  = 1;

  unsigned long int Tbit = 100000/serialSpd;  // Not quite in uSec, I'm saving an extra 10 to change a 1.4 (40%) to 14 below...
  unsigned long int nominalFrameTime = ((34*Tbit)+90*Tbit);  // 90 = 10*max # payload bytes + checksum (9). 
  timeout = LIN_TIMEOUT_IN_FRAMES * 14 * nominalFrameTime;  // 14 is the specced addtl 40% space above normal*10 -- the extra 10 is just pulled out of the 1000000 needed to convert to uSec (so that there are no decimal #s).
}


// Generate a BREAK signal (a low signal for longer than a byte) across the serial line
void Lin::serialBreak(void)
{
  if (serialOn) serial.end();

  pinMode(txPin, OUTPUT);
  digitalWrite(txPin, LOW);  // Send BREAK
  unsigned long int brkend = (1000000UL/((unsigned long int)serialSpd));
  unsigned long int brkbegin = brkend*LIN_BREAK_DURATION;
  if (brkbegin > 16383) delay(brkbegin/1000);  // delayMicroseconds unreliable above 16383 see arduino man pages
  else delayMicroseconds(brkbegin);
  
  digitalWrite(txPin, HIGH);  // BREAK delimiter
 
  if (brkend > 16383) delay(brkend/1000);  // delayMicroseconds unreliable above 16383 see arduino man pages
  else delayMicroseconds(brkend);

  serial.begin(serialSpd);
  serialOn = 1;
}

/* Lin defines its checksum as an inverted 8 bit sum with carry */
uint8_t Lin::dataChecksum(const uint8_t* message, char nBytes,uint16_t sum)
{
    while (nBytes-- > 0) sum += *(message++);
    // Add the carry
    while(sum>>8)  // In case adding the carry causes another carry
      sum = (sum&255)+(sum>>8); 
    return (~sum);
}

/* Create the Lin ID parity */
#define BIT(data,shift) ((addr&(1<<shift))>>shift)
uint8_t Lin::addrParity(uint8_t addr)
{
  uint8_t p0 = BIT(addr,0) ^ BIT(addr,1) ^ BIT(addr,2) ^ BIT(addr,4);
  uint8_t p1 = ~(BIT(addr,1) ^ BIT(addr,3) ^ BIT(addr,4) ^ BIT(addr,5));
  return (p0 | (p1<<1))<<6;
}

/* Send a message across the Lin bus */
void Lin::send(uint8_t addr, const uint8_t* message, uint8_t nBytes,uint8_t proto)
{
  uint8_t addrbyte = (addr&0x3f) | addrParity(addr);
  // LIN diagnostic frame shall always use CHKSUM of protocol version 1.x.
  uint8_t cksum = dataChecksum(message, nBytes, (proto == 1 || addr == 0x3C) ? 0 : addrbyte);
  serialBreak();       // Generate the low signal that exceeds 1 char.
  serial.write(0x55);  // Sync byte
  serial.write(addrbyte);  // ID byte
  serial.write(message,nBytes);  // data bytes
  serial.write(cksum);  // checksum  
}



uint8_t Lin::recv(uint8_t addr, uint8_t* message, uint8_t nBytes,uint8_t proto)
{
  uint8_t bytesRcvd=0;
  unsigned int timeoutCount=0;
  serialBreak();       // Generate the low signal that exceeds 1 char.
  serial.flush();
  serial.write(0x55);  // Sync byte
  uint8_t idByte = (addr&0x3f) | addrParity(addr);
  //p("ID byte %d", idByte);
  serial.write(idByte);  // ID byte
  pinMode(txPin, INPUT);
  digitalWrite(txPin, LOW);  // don't pull up
  do { // I hear myself
    while(!serial.available()) { delayMicroseconds(100); timeoutCount+= 100; if (timeoutCount>=timeout) goto done; }
  } while(serial.read() != 0x55);
  do {
    while(!serial.available()) { delayMicroseconds(100); timeoutCount+= 100; if (timeoutCount>=timeout) goto done; }
  } while(serial.read() != idByte);


  for (uint8_t i=0;i<nBytes;i++)
    {
      // This while loop strategy does not take into account the added time for the logic.  So the actual timeout will be slightly longer then written here.
      while(!serial.available()) { delayMicroseconds(100); timeoutCount+= 100; if (timeoutCount>=timeout) goto done; } 
      message[i] = serial.read();
      bytesRcvd++;
    }
  while(!serial.available()) { delayMicroseconds(100); timeoutCount+= 100; if (timeoutCount>=timeout) goto done; }
  if (serial.available())
    {
    uint8_t cksum = serial.read();
    bytesRcvd++;
    // LIN diagnostic frame shall always use CHKSUM of LIN protocol version 1.x.
    if (proto == 1 || addr == 0x3D) idByte = 0; // Don't cksum the ID byte in LIN 1.x
    if (dataChecksum(message,nBytes,idByte) == cksum) bytesRcvd = 0xff;
    //p("cksum byte %x, calculated %x %x\n",cksum,dataChecksum(message,nBytes,idByte),dataChecksum(message,nBytes,0));
    }

done:
  pinMode(txPin, OUTPUT);

  return bytesRcvd;
}

