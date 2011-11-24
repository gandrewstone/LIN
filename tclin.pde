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


#include "lightuino5.h"
#include "WProgram.h"
#include "lin.h"

Lin lin;

#ifndef SIM
#define simprt
#define assert
#else
#define simprt printf
#include <assert.h>
#endif

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

void setup(void)
{
  lin.begin(19200);
  Usb.begin();
}


void SimpleFrameInjection()
{ 
  if (0)
    {
      uint8_t buf[8];
      uint8_t recvOk = lin.recv(0x0,buf,8,1);  // Should fail since nobody is at 0
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
  //delay(1000); // Don't think the LIN chip is ready right away

  setAllFadeTime(0xe0);
  setAllToColor(255,0,0);
  setAllFadeTime(0xe0);
  Usb.print("Simple LIN frame test\n");
  SimpleFrameInjection(); 
  Usb.print("Scheduler test\n");
  SchedulerTest();
}
