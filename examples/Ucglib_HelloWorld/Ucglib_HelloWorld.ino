/*

  HelloWorld.ino

  A very simple example for Ucglib

  Universal uC Color Graphics Library

  Copyright (c) 2014, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  18 Feb 2014:
    ROM Size	Font Mode
    9928		NONE
    10942		TRANSPARENT	(1014 Bytes)
    11650		SOLID  (1712 Bytes)
    12214		TRANSPARENT+SOLID

   (juh) 14 Jan 2023:
    Modified as example for I2CWrapper Ucglib module

*/

#include <Wire.h>
#include "UcglibI2C.h"

I2Cwrapper wrapper(0x08); // each target device is represented by a wrapper...
UcglibI2C ucg(&wrapper); // ...that the pin interface needs to communicate with the controller

// juh: nearly done here, only a minor change to the font name below,
// otherwise the example code is unchanged from the original


void setup(void)
{
  
  Wire.begin();
  wrapper.setI2Cdelay(12);
  wrapper.reset();
 
  delay(1000);
  ucg.begin(UCG_FONT_MODE_TRANSPARENT); delay(200);
  //ucg.begin(UCG_FONT_MODE_SOLID);
  ucg.clearScreen(); delay(200);

}


void loop(void)
{
  ucg.setRotate90();
  ucg.setFont(I2C_ucg_font_ncenR12_tr); // juh: note that we prefixed the font name with "I2C_"
  ucg.setColor(255, 255, 255);
  //ucg.setColor(0, 255, 0);
  ucg.setColor(1, 255, 0,0);
  
  ucg.setPrintPos(0,25);
  ucg.print("Hello World!");
  ucg.drawVLine(79,0,128);
  ucg.drawVLine(80,0,128);
  ucg.drawHLine(0,63,160);
  ucg.drawHLine(0,64,160);


  delay(500);  
}
