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

/* This example code uses 2 different LIN LEDs of unknown type (acquired as 
   samples).  You will have to modify this code to work with whatever LIN
   devices you have.

   TODO: make a Arduino LIN slave so the code works without modification.
 */

#include "lightuino5.h"
#include "WProgram.h"
#include "lin.h"

Lin lin;

#ifndef SIM
#define simprt p
#define assert
#else
#define simprt printf
#include <stdarg.h>
#include <assert.h>
#endif

// Printf-style to the USB
void p(char *fmt, ... )
{
        char tmp[128]; // resulting string limited to 128 chars
        va_list args;
        va_start (args, fmt );
        vsnprintf(tmp, 128, fmt, args);
        va_end (args);
        Usb.print(tmp);
}


void setAllFadeTime(uint8_t fadeTime)
{
  uint8_t linCmd2[] = { 0x1f, fadeTime };  
  lin.send(0x2e, linCmd2,2,1);
}


void setAllToColor(uint8_t red, uint8_t grn, uint8_t blu)
{
  uint8_t linCmd[] = { blu,grn,red,0xf1 };
  lin.send(0x2f,linCmd,4,1);
  uint8_t linCmd2[] = { 0x1f, 0xe0 };  
  lin.send(0x2e, linCmd2,2,1);
}

void setColor2(uint32_t addr, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness,uint8_t whtLed=0)
{
  uint8_t linCmd[] = { (uint8_t)(addr),(uint8_t)(addr>>8),(uint8_t)(addr>>16),0b00000000 | ((addr>>24)&3),((brightness<<1) | (whtLed&1)),red ,green , blue };
  lin.send(0x01,linCmd,8);
}

void setAllToColor2(uint8_t red, uint8_t blue, uint8_t green, uint8_t brightness)
{
  //unsigned char linCmd[] = { 0xFF,0xFF,0xFF,0b00100111,((brightness<<1) | 0x00),red ,green , blue };
  //lin.send(0x01,linCmd,8);
  for (int i=0;i<10;i++)
    setColor2(0x0fffffff, red,blue,green,brightness);
}


void setup(void)
{
  lin.begin(19200);
  Usb.begin();
}


void SimpleFrameInjection()
{ 
  if (1)
    {
      uint8_t buf[8];
      uint8_t recvOk = lin.recv(32,buf,8,1);  // Should fail since nobody is at 0
      if (recvOk==0) simprt("receive timeout succeeded. ret = %d\n",recvOk);
      else simprt("receive timeout test failed. ret = %d\n",recvOk);
    }

  // Fade all LEDS to red
  for(int i=0;i<255;i+=10)
    {
      setAllToColor(i,0,0);
      delay(60);
    }
  
  // Pop between 2 colors
  for(int i=0;i<255;i+=30)
    {
      setAllToColor(255,0,255);
      delay(500);
      setAllToColor(0,255,0);
      delay(500);
    }
}

class LedFrame:public LinScheduleEntry
{
public:
  LedFrame(uint16_t repeat,uint8_t red, uint8_t grn, uint8_t blu);
  uint8_t  dontUse[3];  // the rest of the data
  uint16_t repeatMs;    // when to repeat 
};

uint16_t LedRepeat(LinScheduleEntry* me)
{
  //Usb.print("led repeat\n");
  return ((LedFrame*) me)->repeatMs;
}

LedFrame::LedFrame(uint16_t rp,uint8_t red, uint8_t grn, uint8_t blu)
  {
    flags    = Lin1Frame | LinWriteFrame;
    callback = LedRepeat;
    repeatMs = rp;
    addr    = 0x2f; 
    data[0] = blu;
    data[1] = grn;
    data[2] = red;
    data[3] = 0xf1;
    len     = 4;
  }


void led2Test(int addr, int delayAmt)
{
  //Usb.print("Test red\n");
  //setAllToColor2(100,1,1,100);
  setColor2(addr,100,0,0,100);
  delay(delayAmt);
  //Usb.print("Test green\n");
  setColor2(addr,0,100,0,100);
  delay(delayAmt);
  //Usb.print("Test blue\n");
  setAllToColor2(0,0,100,100);
  delay(delayAmt);

#if 0    
  Usb.print("Fading\n");

  for (int i=10;i<=100;i+=10)
    {
      delay(50);
      setAllToColor2(100,100,100,i);
    }


  for (int i=10;i<=100;i+=10)
    {
      delay(50);
      setAllToColor2(100-i,100,100,100);
    }
    
  for (int i=10;i<100;i+=10)
    {
      delay(50);
      setAllToColor2(1,100-i,100,100);
    }
    
  for (int i=10;i<100;i+=10)
    {
      delay(50);
      setAllToColor2(1,1,100-i,100);
    }
#endif
}

void led2RecvTest(uint8_t addr)
{

  uint8_t buf[8];
  setColor2((1<<(addr-1)),100,0,0,0x60);
  for (int i=0;i<8;i++) buf[i] = 0;
  delay(400);
  uint8_t recvOk = lin.recv(0x20 + addr-1,buf,6,1);  // The data is correct but the checksum byte is 0xFF -- may be a bug in the device
  p("received %d bytes. %2x %2x %2x %2x %2x %2x %2x %2x\n",recvOk, buf[0],buf[1],buf[2],buf[3], buf[4],buf[5],buf[6],buf[7]);
  setColor2((1<<(addr-1)),0,0x40,0,100);
  for (int i=0;i<8;i++) buf[i] = 0;
  delay(300);
  recvOk = lin.recv(0x20 + addr-1,buf,6,1);
  p("received %d bytes. %2x %2x %2x %2x %2x %2x %2x %2x\n",recvOk, buf[0],buf[1],buf[2],buf[3], buf[4],buf[5],buf[6],buf[7]);

  delay(100);
  //setColor2((1<<(addr-1)),00,100,0,100);
  for (int i=0;i<8;i++) buf[i] = 0;
  delay(200);
  recvOk = lin.recv(0x21 + addr-1,buf,6,1);  // Should fail
  p("received %d bytes. %2x %2x %2x %2x %2x %2x %2x %2x\n",recvOk, buf[0],buf[1],buf[2],buf[3], buf[4],buf[5],buf[6],buf[7]);

  //if (recvOk==0) simprt("receive timeout. ret = %d\n",recvOk);
  //else simprt("receive ok. ret = %d\n",recvOk);
}

void SchedulerTest(void)
{
  unsigned long int start = millis();

  LedFrame red(1500,0xff,0,0);
  LedFrame grn(1500,0,0xff,0);
  LedFrame blu(1500,0,0,0xff);
  lin.add(red,1); lin.add(grn,500); lin.add(blu,1000);
  
  while (millis() < start + 10000) // 10 second test
    {
      lin.loop();
      delay(1);
    }
  lin.clear();  // Better remove them before I drop out of this stack frame!
}


 void loop(void)
{
  uint8_t addr = 5;
  for (int delayAmt = 301; delayAmt>0;delayAmt-=100)
    led2Test((1<<(addr-1)),delayAmt);
  led2RecvTest(addr);

  setAllFadeTime(0xe0);
  setAllToColor(255,0,0);
  setAllFadeTime(0xe0);
  Usb.print("Simple LIN frame test\n");
  SimpleFrameInjection(); 
  Usb.print("Scheduler test\n");
  SchedulerTest();
}
