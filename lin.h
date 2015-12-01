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


//typedef unsigned char uint8_t;
//typedef unsigned int uint16_t;
#include <inttypes.h>
#include "Arduino.h"
#include <HardwareSerial.h>

#define LIN_SERIAL            HardwareSerial
#define LIN_BREAK_DURATION    15    // Number of bits in the break.
#define LIN_TIMEOUT_IN_FRAMES 2     // Wait this many max frame times before declaring a read timeout.

#include "heapskew.h"

// This code allows you to derive from the LINFrame class to define messages of different lengths and therefore save a few bytes of RAM.  But it relies on the compiler laying out the class memory in RAM so that the derived class data directly follows the base class.
// If this is not the case in your system, the easiest way to get something working is to only allow full frames, as shown in the #else clause.
// Also, you can over-malloc the LINFrame class to create an extended data buffer.  Of course this method has its own memory management overhead.

enum
  {
    Lin1Frame = 0,
    Lin2Frame = 1,
    LinWriteFrame  = 0,
    LinReadFrame   = 2,
  };

#if 1
class LinScheduleEntry
{
public:
  LinScheduleEntry();
  unsigned long int trigger;  // When to execute (if it is scheduled)
  HeapSkew<LinScheduleEntry>::SkewHeapElem skewChildren;
  uint16_t (*callback)(LinScheduleEntry* me);  // Called after this frame is processed.  Return 1 or greater to reschedule the frame that many milliseconds from now, 0 to drop it.  if callback is NULL, frame is considered a one-shot.
  uint8_t   flags;  
  uint8_t   addr;
  uint8_t   len;
  uint8_t   data[1];
  // The data bytes must follow the len field in RAM

  // So that the skew heap orders the elements by when they should be executed
  int operator > (LinScheduleEntry& ee)
  {
    return trigger > ee.trigger;
  }

};

class LinSeFullFrame:public LinScheduleEntry
{
public:
  uint8_t  dontUse[7];  // Code will actually write this by overwriting the "data" buffer in the base class.
};

#else

class LinScheduleEntry
{
public:
  LinScheduleEntry();
  HeapSkew<EnsembleElem>::SkewHeapElem skewChildren;
  uint8_t   addr;
  uint8_t   len;
  uint8_t   data[8];
};

#define LinSeFullFrame LinScheduleEntry
#endif


class Lin
{
protected:
  void serialBreak(void);
  // For Lin 1.X "start" should = 0, for Lin 2.X "start" should be the addr byte. 
  static uint8_t dataChecksum(const uint8_t* message, char nBytes,uint16_t start=0);
  static uint8_t addrParity(uint8_t addr);

  HeapSkew<LinScheduleEntry> scheduler;

public:
  Lin(LIN_SERIAL& ser=Serial,uint8_t txPin=1);
  LIN_SERIAL& serial;
  uint8_t txPin;               //  what pin # is used to transmit (needed to generate the BREAK signal)
  int     serialSpd;           //  in bits/sec. Also called baud rate
  uint8_t serialOn;            //  whether the serial port is "begin"ed or "end"ed.  Optimization so we don't begin twice.
  unsigned long int timeout;   //  How long to wait for a slave to fully transmit when doing a "read".  You can modify this after calling "begin"
  void begin(int speed);

  // Send a message right now, ignoring the schedule table.
  void send(uint8_t addr, const uint8_t* message,uint8_t nBytes,uint8_t proto=2);

  // Receive a message right now, returns 0xff if good checksum, # bytes received (including checksum) if checksum is bad.
  uint8_t recv(uint8_t addr, uint8_t* message, uint8_t nBytes,uint8_t proto=2);

  // Scheduler APIs

  // Add an element to the schedule.  To remove, either clear the whole thing, or remove it when it next plays
  void add(LinScheduleEntry& entry,uint16_t when=0) { entry.trigger = millis() + when; scheduler.push(entry); }
  void clear(void) { scheduler.clear(); }
  void loop();

};
