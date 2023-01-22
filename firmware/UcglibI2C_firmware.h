/*!
   @file UcglibI2C_firmware.h
   @brief I2Cwrapper module for adressing TFT and other displays supported by
   Ucglib over I2C.

   Be sure to adapt the display type and connections in the "(2) declarations"
   stage below, according to the specific hardware your target device has attached.

   See README.md for limitations.

   ## Author
   Copyright (c) 2022 juh
   ## License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/


/// @cond

/*
   (1) includes
*/
#if MF_STAGE == MF_STAGE_includes

#include <SPI.h>
#include "Ucglib.h"
#include <UcglibI2C.h>

#endif // MF_STAGE_includes


/*
   (2) declarations
*/
#if MF_STAGE == MF_STAGE_declarations

struct UcglibFontListEntry {
  const ucg_fntpgm_uint8_t* font;
  UcglibI2C_Font id;
};

// ======================================================
// ================ start of user config ================
// ======================================================

// (1) Specify the target pins your display is connected to:

const uint8_t SPIpinCS = A2;
const uint8_t SPIpinDC = A1;
const uint8_t SPIpinReset = A0; // set to 0 if not existig


// (2) Specify the type of display, see Ucglib documentation/examples for a list:

Ucglib_ST7735_18x128x160_HWSPI ucg(/*cd=*/ SPIpinDC, /*cs=*/ SPIpinCS, /*reset=*/ SPIpinReset);
// Ucglib provides also sofware SPI ("_SWSPI"), which is not recommended here for speed reasons


// (3) Undefine this if your display doesn't have a reset pin, if it is not connected to an I2C target pin or if it is directly connected to the target's own reset pin:
#define UcglibI2CresetDisplay LOW // set this to HIGH if the display reset pin is active HIGH


// (4) Specify the fonts available:
/*
   Note: Only a limited number of preselected fonts will fit into flash memory of this I2C-target.
   So this is the list of fonts that will be available. Enter them as pairs, once without
   leading "I2C_", once with leading "I2C_", the latter will be the name to be used on the
   controller's side.
*/

UcglibFontListEntry availableFonts[] = {
  { ucg_font_ncenR12_tr, I2C_ucg_font_ncenR12_tr } ,
  { ucg_font_helvB08_hr, I2C_ucg_font_helvB08_hr },
  { ucg_font_helvB10_hr, I2C_ucg_font_helvB10_hr },
  { ucg_font_helvB12_hr, I2C_ucg_font_helvB12_hr },
  { ucg_font_ncenR14_hr, I2C_ucg_font_ncenR14_hr },
  { ucg_font_helvB18_hr, I2C_ucg_font_helvB18_hr }
};

// ======================================================
// ================ end of user config ================
// ======================================================


const uint8_t numberOfAvailableFonts = sizeof(availableFonts) / sizeof(UcglibFontListEntry);


void resetDisplay() {

#if defined(UcglibI2CresetDisplay)

  digitalWrite(SPIpinReset, UcglibI2CresetDisplay);
  delay(10); // ST7735 reset will already start after 5µs, I assume with 10ms we are on the safe side for other displays as well
  digitalWrite(SPIpinReset, not UcglibI2CresetDisplay); // not HIGH == LOW

#else

  // there is no software reset in Ucglib, I think this is the closest equivalent
  ucg.begin(UCG_FONT_MODE_NONE);
  ucg.clearScreen();

#endif // defined(UcglibI2CresetDisplay)

}

#endif // MF_STAGE_declarations


/*
   (3) setup() function
*/
#if MF_STAGE == MF_STAGE_setup
log("UcglibI2C module enabled.\n");
resetDisplay();
//ucg.begin(UCG_FONT_MODE_TRANSPARENT);
//ucg.clearScreen();
////ucg.setRotate180();
////ucg.setColor(0x65, 0xa1, 0xaa);
//ucg.setFont(availableFonts[0].font);
//ucg.setPrintPos(0, 25);
//ucg.print("UcglibI2C ready.");
//delay(500);
//ucg.clearScreen();

#endif // MF_STAGE_setup


/*
   (4) main loop() function
*/
#if MF_STAGE == MF_STAGE_loop
#endif // MF_STAGE_loop


/*
   (5) processMessage() function
*/
#if MF_STAGE == MF_STAGE_processMessage

case UcglibBeginCmd: {
  if (i == 1) {
    uint8_t is_transparent;
    bufferIn->read(is_transparent);
    ucg.begin(is_transparent);
  }
}
break;

case UcglibClearScreenCmd: {
  if (i == 0) {
    ucg.clearScreen();
  }
}
break;

case UcglibSetFontCmd: {
  if (i == 2) { // enum UcglibI2C_Font is of type uint16_t
    UcglibI2C_Font fontID;
    bufferIn->read(fontID);
    uint8_t j = 0; // search for font in list of available fonts
    while ((j < numberOfAvailableFonts) and (availableFonts[j].id != fontID)) {
      j++;
    }
    if (j == numberOfAvailableFonts) {// font not found, substitute by first one
      j = 0;
    }
    ucg.setFont(availableFonts[j].font);
  }
}
break;

case UcglibSetColorCmd: {
  if (i == 4) {
    uint8_t idx, r, g, b;
    bufferIn->read(idx);
    bufferIn->read(r); bufferIn->read(g); bufferIn->read(b);
    ucg.setColor(idx, r, g, b);
  }
}
break;

case UcglibSetPrintPosCmd: {
  if (i == 4) { // typedef int16_t ucg_int_t;
    ucg_int_t x, y;
    bufferIn->read(x);
    bufferIn->read(y);
    ucg.setPrintPos(x, y);
  }
}
break;

case UcglibWriteCmd: {
  if (i == 1) { // 1 char
    uint8_t c;
    bufferIn->read(c);
    ucg.write(c);
  }
}
break;

case UcglibSettingCmd: {
  if (i == 1) {  // subcommand = 1 uint8_t
    uint8_t subcommand;
    bufferIn->read(subcommand);
    switch (subcommand) {
      case UcglibSettingCmdRotate0:
        ucg.undoRotate();
        break;
      case UcglibSettingCmdRotate90:
        ucg.setRotate90();
        break;
      case UcglibSettingCmdRotate180:
        ucg.setRotate180();
        break;
      case UcglibSettingCmdRotate270:
        ucg.setRotate270();
        break;
      case UcglibSettingCmdSetFontRefHeightText:
        ucg.setFontRefHeightText();
        break;
      case UcglibSettingCmdSetFontRefHeightExtendedText:
        ucg.setFontRefHeightExtendedText();
        break;
      case UcglibSettingCmdSetFontRefHeightAll:
        ucg.setFontRefHeightAll();
        break;
      case UcglibSettingCmdSetFontPosBaseline:
        ucg.setFontPosBaseline();
        break;
      case UcglibSettingCmdSetFontPosBottom:
        ucg.setFontPosBottom();
        break;
      case UcglibSettingCmdSetFontPosTop:
        ucg.setFontPosTop();
        break;
      case UcglibSettingCmdSetFontPosCenter:
        ucg.setFontPosCenter();
        break;
      case UcglibSettingCmdUndoScale:
        ucg.undoScale();
        break;
      case UcglibSettingCmdSetScale2x2:
        ucg.setScale2x2();
        break;
      case UcglibSettingCmdPowerDown:
        ucg.powerDown();
        break;
      case UcglibSettingCmdPowerUp:
        ucg.powerUp();
        break;
      case UcglibSettingCmdSetMaxClipRange:
        ucg.setMaxClipRange();
        break;
      case UcglibSettingCmdUndoClipRange:
        ucg.undoClipRange();
        break;
    }
  }
}
break;

case Ucglib1uint8_tCmd: {
  if (i == 2) { // subcommand = 1 uint8_t + 1 uint8_t
    uint8_t subcommand;
    uint8_t p1;
    bufferIn->read(subcommand);
    bufferIn->read(p1);
    switch (subcommand) {
      case Ucglib1uint8_tCmdSetPrintDir:
        ucg.setPrintDir(p1);
        break;
      case Ucglib1uint8_tCmdSetFontMode:
        ucg.setFontMode(p1);
        break;
    }
  }
}
break;

case UcglibGetCmd: {
  ucg_int_t getCmdResult = -1;
  if (i == 1) { // subcommand = 1 uint8_t
    uint8_t subcommand;
    bufferIn->read(subcommand);
    switch (subcommand) {
      case UcglibGetCmdGetWidth:
        getCmdResult = ucg.getWidth();
        break;
      case UcglibGetCmdGetHeight:
        getCmdResult = ucg.getHeight();
        break;
      case UcglibGetCmdGetFontAscent:
        getCmdResult = ucg.getFontAscent();
        break;
      case UcglibGetCmdGetFontDescent:
        getCmdResult = ucg.getFontDescent();
        break;
    }
  }
  bufferOut->write(getCmdResult);
}
break;


case UcglibGetStrWidthCmd: {
  ucg_int_t getStrWidthResult = -1;
  if (i >= 1) { // we can't know the exact length, yet, only that we need at least a length byte
    uint8_t len;
    bufferIn->read(len);
    if (i == len + 1) { // now we can do the real check, +1 is for lenght byte
      if (len < I2CmaxBuf - 4) {
        char *s = new char[len];
        for (uint8_t j = 0; j < len; j++) {
          bufferIn->read(s[j]);
        }
        if (s[len - 1] == 0x0) { // Arnold found at the end?
          getStrWidthResult = ucg.getStrWidth(s);
        }
      }
    }
  }
  bufferOut->write(getStrWidthResult);
}
break;


case Ucglib4ucg_int_tCmd: {
  if (i == 9) { // 4 * typedef int16_t ucg_int_t + 1 subcommand
    uint8_t subcommand;
    ucg_int_t p1, p2, p3, p4;
    bufferIn->read(subcommand);
    bufferIn->read(p1);
    bufferIn->read(p2);
    bufferIn->read(p3);
    bufferIn->read(p4);
    switch (subcommand) {
      case Ucglib4ucg_int_tCmdSetClipRange:
        ucg.setClipRange(p1, p2, p3, p4);
        break;
      case Ucglib4ucg_int_tCmdDrawLine:
        ucg.drawLine(p1, p2, p3, p4);
        break;
      case Ucglib4ucg_int_tCmdDrawBox:
        ucg.drawBox(p1, p2, p3, p4);
        break;
      case Ucglib4ucg_int_tCmdDrawFrame:
        ucg.drawFrame(p1, p2, p3, p4);
        break;
      case Ucglib4ucg_int_tCmdDrawGradientLine:
        ucg.drawGradientLine(p1, p2, p3, p4);
        break;
      case Ucglib4ucg_int_tCmdDrawGradientBox:
        ucg.drawGradientBox(p1, p2, p3, p4);
        break;
    }
  }
}
break;


case UcglibDrawPixelCmd: {
  if (i == 4) { // typedef int16_t ucg_int_t;
    ucg_int_t x, y;
    bufferIn->read(x);
    bufferIn->read(y);
    ucg.drawPixel(x, y);
  }
}
break;


case Ucglib3ucg_int_tCmd: {
  if (i == 7) { // 3 * typedef int16_t ucg_int_t + 1 subcommand
    uint8_t subcommand;
    ucg_int_t p1, p2, p3;
    bufferIn->read(subcommand);
    bufferIn->read(p1);
    bufferIn->read(p2);
    bufferIn->read(p3);
    switch (subcommand) {
      case Ucglib3ucg_int_tCmdDrawHLine:
        ucg.drawHLine(p1, p2, p3);
        break;
      case Ucglib3ucg_int_tCmdDrawVLine:
        ucg.drawVLine(p1, p2, p3);
        break;
    }
  }
}
break;


case Ucglib5ucg_int_tCmd: {
  if (i == 11) { // 5 * typedef int16_t ucg_int_t + 1 subcommand
    uint8_t subcommand;
    ucg_int_t p1, p2, p3, p4, p5;
    bufferIn->read(subcommand);
    bufferIn->read(p1);
    bufferIn->read(p2);
    bufferIn->read(p3);
    bufferIn->read(p4);
    bufferIn->read(p5);
    switch (subcommand) {
      case Ucglib5ucg_int_tCmdDrawRBox:
        ucg.drawRBox(p1, p2, p3, p4, p5);
        break;
      case Ucglib5ucg_int_tCmdDrawRFrame:
        ucg.drawRFrame(p1, p2, p3, p4, p5);
        break;
    }
  }
}
break;


case UcglibDrawGlyphCmd: {
  ucg_int_t drawGlyphResult = -1;
  if (i == 6) { // ucg_int_t x, ucg_int_t y, uint8_t dir, uint8_t encoding
    uint8_t subcommand;
    ucg_int_t x;
    ucg_int_t y;
    uint8_t dir;
    uint8_t encoding;
    bufferIn->read(subcommand);
    bufferIn->read(x);
    bufferIn->read(y);
    bufferIn->read(dir);
    bufferIn->read(encoding);
    drawGlyphResult = ucg.drawGlyph(x, y, dir, encoding);
  }
  bufferOut->write(drawGlyphResult);
}
break;


case UcglibDrawStringCmd: {
  ucg_int_t drawStringResult = -1;
  if (i >= 6) { // ucg_int_t x (2) + ucg_int_t y (2) + uint8_t dir (1) + 1 len byte (1) for const char *str
    ucg_int_t x;
    ucg_int_t y;
    uint8_t dir;
    uint8_t len;
    bufferIn->read(x);
    bufferIn->read(y);
    bufferIn->read(dir);
    bufferIn->read(len);
    if (i == len + 6) { // now we can do the real check,
      if (len < I2CmaxBuf - 4) { ///####4?
        char *str = new char[len];
        for (uint8_t j = 0; j < len; j++) {
          bufferIn->read(str[j]);
        }
        if (str[len - 1] == 0x0) { // Arnold found at the end?
          drawStringResult = ucg.drawString(x, y, dir, str);
        }
      }
    }
  }
  bufferOut->write(drawStringResult);
}
break;


case UcglibDrawWithRadiusCmd: {
  if (i == 8) { // uint8_t subcommand, ucg_int_t x0, ucg_int_t y0, ucg_int_t rad, uint8_t option
    uint8_t subcommand;
    ucg_int_t x0;
    ucg_int_t y0;
    ucg_int_t rad;
    uint8_t option;
    bufferIn->read(subcommand);
    bufferIn->read(x0);
    bufferIn->read(y0);
    bufferIn->read(rad);
    bufferIn->read(option);
    switch (subcommand) {
      case UcglibDrawWithRadiusCmdDrawDisc:
        ucg.drawDisc(x0, y0, rad, option);
        break;
      case UcglibDrawWithRadiusCmdDrawCircle:
        ucg.drawCircle(x0, y0, rad, option);
        break;
    }
  }
}
break;


case UcglibDrawTriangleCmd: {
  if (i == 12) { // int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
    bufferIn->read(x0);
    bufferIn->read(y0);
    bufferIn->read(x1);
    bufferIn->read(y1);
    bufferIn->read(x2);
    bufferIn->read(y2);
    ucg.drawTriangle(x0, y0, x1, y1, x2, y2);
  }
}
break;

case UcglibDrawTetragonCmd: {
  if (i == 16) { // int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3
    uint8_t subcommand;
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
    int16_t x3;
    int16_t y3;
    bufferIn->read(x0);
    bufferIn->read(y0);
    bufferIn->read(x1);
    bufferIn->read(y1);
    bufferIn->read(x2);
    bufferIn->read(y2);
    bufferIn->read(x3);
    bufferIn->read(y3);
    ucg.drawTetragon(x0, y0, x1, y1, x2, y2, x3, y3);
  }
}
break;



#endif // MF_STAGE_processMessage


/*
   (6) reset event

   This code will be called if the target device is to perform a (soft) reset
   via I2C command. Here, the module needs to free any ressources it has allocated
   and put any hardware under its rule (and only that!) in its initial state.

   Note: From v0.3.0  on, no hardware reset is performed any more. So it is
   essential that after calling this code, the target device presents itself as
   a clean slate to the controller, just like after power up.
*/
#if MF_STAGE == MF_STAGE_reset
resetDisplay();
#endif // MF_STAGE_reset


/*
   (7) receiveEvent()

   Normal modules usually should not mess around here.

*/
#if MF_STAGE == MF_STAGE_receiveEvent
#endif // MF_STAGE_receiveEvent


/*
   (8) requestEvent()

   Normal modules usually should not mess around here.

*/
#if MF_STAGE == MF_STAGE_requestEvent
#endif // MF_STAGE_requestEvent


/*
   (9) Change of I2C state machine's state

   Normal modules usually should not mess around here.

*/

#if MF_STAGE == MF_STAGE_I2CstateChange
#endif // MF_STAGE_I2CstateChange




/// @endcond
