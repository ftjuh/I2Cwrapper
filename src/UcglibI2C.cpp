/*!
  @file UcglibI2C.cpp
  @brief 
  
  ## Author
  Copyright (c) 2022 juh
  ## License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/

#ifndef ARDUINO_ARCH_STM32 // it seems Ucglib is not compatible with stm32duino

#include <UcglibI2C.h>

const uint8_t myNum = 253; // currently a fake myNum, for only one TFT
// todo: implement units, so that more than one display can be addressed by one I2C-target

// Constructor
UcglibI2C::UcglibI2C(I2Cwrapper* w)
{
  wrapper = w;
}

void UcglibI2C::begin(uint8_t is_transparent) {
  wrapper->prepareCommand(UcglibBeginCmd, myNum);
  wrapper->buf.write(is_transparent);
  wrapper->sendCommand();
}

void UcglibI2C::clearScreen(void) {
  wrapper->prepareCommand(UcglibClearScreenCmd, myNum);
  wrapper->sendCommand();
}

void UcglibI2C::setFont(UcglibI2C_Font id) {
  wrapper->prepareCommand(UcglibSetFontCmd, myNum);
  wrapper->buf.write(id);
  wrapper->sendCommand();
}


void UcglibI2C::setColor(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
  wrapper->prepareCommand(UcglibSetColorCmd, myNum);
  wrapper->buf.write(idx);
  wrapper->buf.write(r); wrapper->buf.write(g); wrapper->buf.write(b);
  wrapper->sendCommand();
}


void UcglibI2C::setColor(uint8_t r, uint8_t g, uint8_t b) {
  setColor(0, r, g, b); // idx defaults to 0, see https://github.com/olikraus/ucglib/wiki/reference#setcolor
}


void UcglibI2C::setPrintPos(ucg_int_t x, ucg_int_t y) {
  wrapper->prepareCommand(UcglibSetPrintPosCmd, myNum);
  wrapper->buf.write(x);
  wrapper->buf.write(y);
  wrapper->sendCommand();
}


size_t UcglibI2C::write(uint8_t c) {
  wrapper->prepareCommand(UcglibWriteCmd, myNum);
  wrapper->buf.write(c);
  wrapper->sendCommand();
  return 1; // that's what the Ucglib::write() returns, too, so we don't bother to get it via I2C
}


void UcglibI2C::settingCmd(uint8_t subcommand) {
  wrapper->prepareCommand(UcglibSettingCmd, myNum);
  wrapper->buf.write(subcommand);
  wrapper->sendCommand();
}
void UcglibI2C::undoRotate() {
  settingCmd(UcglibSettingCmdRotate0);
}
void UcglibI2C::setRotate90() {
  settingCmd(UcglibSettingCmdRotate90);
}
void UcglibI2C::setRotate180() {
  settingCmd(UcglibSettingCmdRotate180);
}
void UcglibI2C::setRotate270() {
  settingCmd(UcglibSettingCmdRotate270);
}
void UcglibI2C::setFontRefHeightText() {
  settingCmd(UcglibSettingCmdSetFontRefHeightText);
}
void UcglibI2C::setFontRefHeightExtendedText() {
  settingCmd(UcglibSettingCmdSetFontRefHeightExtendedText);
}
void UcglibI2C::setFontRefHeightAll() {
  settingCmd(UcglibSettingCmdSetFontRefHeightAll);
}
void UcglibI2C::setFontPosBaseline() {
  settingCmd(UcglibSettingCmdSetFontPosBaseline);
}
void UcglibI2C::setFontPosBottom() {
  settingCmd(UcglibSettingCmdSetFontPosBottom);
}
void UcglibI2C::setFontPosTop() {
  settingCmd(UcglibSettingCmdSetFontPosTop);
}
void UcglibI2C::setFontPosCenter() {
  settingCmd(UcglibSettingCmdSetFontPosCenter);
}
void UcglibI2C::undoScale() {
  settingCmd(UcglibSettingCmdUndoScale);
}
void UcglibI2C::setScale2x2() {
  settingCmd(UcglibSettingCmdSetScale2x2);
}
void UcglibI2C::powerDown() {
  settingCmd(UcglibSettingCmdPowerDown);
}
void UcglibI2C::powerUp() {
  settingCmd(UcglibSettingCmdPowerUp);
}
void UcglibI2C::setMaxClipRange() {
  settingCmd(UcglibSettingCmdSetMaxClipRange);
}
void UcglibI2C::undoClipRange() {
  settingCmd(UcglibSettingCmdUndoClipRange);
}


void UcglibI2C::OneUint8_tCmd(uint8_t subcommand, 
                              uint8_t p1) {
  wrapper->prepareCommand(Ucglib1uint8_tCmd, myNum);
  wrapper->buf.write(subcommand);
  wrapper->buf.write(p1);
  wrapper->sendCommand();
}
void UcglibI2C::setPrintDir(uint8_t dir) {
  OneUint8_tCmd(Ucglib1uint8_tCmdSetPrintDir, dir);
}
void UcglibI2C::setFontMode(uint8_t is_transparent) {
  OneUint8_tCmd(Ucglib1uint8_tCmdSetFontMode, is_transparent);
}


ucg_int_t UcglibI2C::GetCmd(uint8_t subcommand) {
  wrapper->prepareCommand(UcglibGetCmd, myNum);
  wrapper->buf.write(subcommand);
  ucg_int_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(UcglibGetCmdResult)) {
    wrapper->buf.read(res);
  }
  return res;  
}
ucg_int_t UcglibI2C::getWidth() {
  return GetCmd(UcglibGetCmdGetWidth);
}
ucg_int_t UcglibI2C::getHeight() {
  return GetCmd(UcglibGetCmdGetHeight);
}
ucg_int_t UcglibI2C::getFontAscent() {
  return GetCmd(UcglibGetCmdGetFontAscent);
}
ucg_int_t UcglibI2C::getFontDescent() {
  return GetCmd(UcglibGetCmdGetFontDescent);
}


ucg_int_t UcglibI2C::getStrWidth(const char *s) {
  ucg_int_t res = -1;
  uint8_t len = uint8_t(strlen(s));
  if ((len > 0) and (len < I2CmaxBuf - 4)) { // command header + 1 len byte = 4
    wrapper->prepareCommand(UcglibGetStrWidthCmd, myNum);
    wrapper->buf.write(uint8_t(len + 1)); // so the target knows how may chars to expect
    for (uint8_t i = 0; i <= len; i++) { // include terminating 0x0
      wrapper->buf.write(s[i]);
    }
    if (wrapper->sendCommand() and wrapper->readResult(UcglibGetStrWidthCmdResult)) {
      wrapper->buf.read(res);
    }
  }
  return res;
}


void UcglibI2C::FourUcg_int_tCmd(uint8_t subcommand, 
                                 ucg_int_t p1, ucg_int_t p2, ucg_int_t p3, ucg_int_t p4) {
  wrapper->prepareCommand(Ucglib4ucg_int_tCmd, myNum);
  wrapper->buf.write(subcommand);
  wrapper->buf.write(p1); 
  wrapper->buf.write(p2);
  wrapper->buf.write(p3);
  wrapper->buf.write(p4);
  wrapper->sendCommand();
}
void UcglibI2C::setClipRange (ucg_int_t x,  ucg_int_t y,  ucg_int_t w,   ucg_int_t h) {
  FourUcg_int_tCmd(Ucglib4ucg_int_tCmdSetClipRange, x, y, w, h);
}
void UcglibI2C::drawLine (ucg_int_t x1, ucg_int_t y1, ucg_int_t x2,  ucg_int_t y2) {
  FourUcg_int_tCmd(Ucglib4ucg_int_tCmdDrawLine, x1, y1, x2, y2);
} 
void UcglibI2C::drawBox (ucg_int_t x,  ucg_int_t y,  ucg_int_t w,   ucg_int_t h) {
  FourUcg_int_tCmd(Ucglib4ucg_int_tCmdDrawBox, x, y, w, h);
}
void UcglibI2C::drawFrame (ucg_int_t x,  ucg_int_t y,  ucg_int_t w,   ucg_int_t h) {
  FourUcg_int_tCmd(Ucglib4ucg_int_tCmdDrawFrame, x, y, w, h);
}
void UcglibI2C::drawGradientLine (ucg_int_t x,  ucg_int_t y,  ucg_int_t len, ucg_int_t dir) {
  FourUcg_int_tCmd(Ucglib4ucg_int_tCmdDrawGradientLine, x, y, len, dir);
}
void UcglibI2C::drawGradientBox (ucg_int_t x,  ucg_int_t y,  ucg_int_t w,   ucg_int_t h) {
  FourUcg_int_tCmd(Ucglib4ucg_int_tCmdDrawGradientBox, x, y, w, h);
}


void UcglibI2C::drawPixel(ucg_int_t x, ucg_int_t y) {
  wrapper->prepareCommand(UcglibDrawPixelCmd, myNum);
  wrapper->buf.write(x);
  wrapper->buf.write(y);
  wrapper->sendCommand();
}


void UcglibI2C::ThreeUcg_int_tCmd(uint8_t subcommand, ucg_int_t p1, ucg_int_t p2, ucg_int_t p3) {
  wrapper->prepareCommand(Ucglib3ucg_int_tCmd, myNum);
  wrapper->buf.write(subcommand);
  wrapper->buf.write(p1); 
  wrapper->buf.write(p2);
  wrapper->buf.write(p3);
  wrapper->sendCommand();
}
void UcglibI2C::drawHLine(ucg_int_t x,  ucg_int_t y,  ucg_int_t len) {
  ThreeUcg_int_tCmd(Ucglib3ucg_int_tCmdDrawHLine, x, y, len);
}
void UcglibI2C::drawVLine(ucg_int_t x,  ucg_int_t y,  ucg_int_t len) {
  ThreeUcg_int_tCmd(Ucglib3ucg_int_tCmdDrawVLine, x, y, len);
}


void UcglibI2C::FiveUcg_int_tCmd(uint8_t subcommand, ucg_int_t p1, ucg_int_t p2, ucg_int_t p3, ucg_int_t p4, ucg_int_t p5) {
  wrapper->prepareCommand(Ucglib5ucg_int_tCmd, myNum);
  wrapper->buf.write(subcommand);
  wrapper->buf.write(p1); 
  wrapper->buf.write(p2);
  wrapper->buf.write(p3);
  wrapper->buf.write(p4);
  wrapper->buf.write(p5);
  wrapper->sendCommand();
}
void UcglibI2C::drawRBox(ucg_int_t x, ucg_int_t y, ucg_int_t w, ucg_int_t h, ucg_int_t r) {
  FiveUcg_int_tCmd(Ucglib5ucg_int_tCmdDrawRBox, x, y, w, h, r);
}
void UcglibI2C::drawRFrame(ucg_int_t x, ucg_int_t y, ucg_int_t w, ucg_int_t h, ucg_int_t r) {
  FiveUcg_int_tCmd(Ucglib5ucg_int_tCmdDrawRFrame, x, y, w, h, r);
}


ucg_int_t UcglibI2C::drawGlyph(ucg_int_t x, ucg_int_t y, uint8_t dir, uint8_t encoding) {
  wrapper->prepareCommand(UcglibDrawGlyphCmd, myNum);
  wrapper->buf.write(x); 
  wrapper->buf.write(y);
  wrapper->buf.write(dir);
  wrapper->buf.write(encoding);
  ucg_int_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(UcglibDrawGlyphCmdResult)) {
    wrapper->buf.read(res);
  }
  return res;
  
}


// max 255 characters
ucg_int_t UcglibI2C::drawString(ucg_int_t x, ucg_int_t y, uint8_t dir, const char *str) {
  uint8_t len = uint8_t(strlen(str));
  if ((len > 0) and (len < I2CmaxBuf - 4)) { // ###command header + 1 len byte = 4
    wrapper->prepareCommand(UcglibDrawStringCmd, myNum);
    wrapper->buf.write(x); 
    wrapper->buf.write(y);
    wrapper->buf.write(dir);  
    wrapper->buf.write(uint8_t(len + 1)); // so the target knows how may chars to expect
    for (uint8_t i = 0; i <= len; i++) { // include terminating 0x0
      wrapper->buf.write(str[i]);
    }
    ucg_int_t res = -1;
    if (wrapper->sendCommand() and wrapper->readResult(UcglibDrawStringCmdResult)) {
      wrapper->buf.read(res);
    }
    return res;
  } else {
    return 0;
  }
}



void UcglibI2C::DrawWithRadius(uint8_t subcommand,
                               ucg_int_t x0, ucg_int_t y0, ucg_int_t rad, uint8_t option) {
  wrapper->prepareCommand(UcglibDrawWithRadiusCmd, myNum);
  wrapper->buf.write(subcommand);
  wrapper->buf.write(x0);
  wrapper->buf.write(y0);
  wrapper->buf.write(rad);
  wrapper->buf.write(option);
  wrapper->sendCommand();
}
void UcglibI2C::drawDisc (ucg_int_t x0, ucg_int_t y0, ucg_int_t rad, uint8_t option) {
  DrawWithRadius(UcglibDrawWithRadiusCmdDrawDisc, x0, y0, rad, option);
}
void UcglibI2C::drawCircle(ucg_int_t x0, ucg_int_t y0, ucg_int_t rad, uint8_t option) {
  DrawWithRadius(UcglibDrawWithRadiusCmdDrawCircle, x0, y0, rad, option);
}


void UcglibI2C::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  wrapper->prepareCommand(UcglibDrawTriangleCmd, myNum);
  wrapper->buf.write(x0); 
  wrapper->buf.write(y0);
  wrapper->buf.write(x1);
  wrapper->buf.write(y1);
  wrapper->buf.write(x2);
  wrapper->buf.write(y2);
  wrapper->sendCommand();
}


void UcglibI2C::drawTetragon(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3) {
  wrapper->prepareCommand(UcglibDrawTetragonCmd, myNum);
  wrapper->buf.write(x0); 
  wrapper->buf.write(y0);
  wrapper->buf.write(x1);
  wrapper->buf.write(y1);
  wrapper->buf.write(x2);
  wrapper->buf.write(y2);
  wrapper->buf.write(x3);
  wrapper->buf.write(y3);
  wrapper->sendCommand();
}

#endif // ARDUINO_ARCH_STM32
