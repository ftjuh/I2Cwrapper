/*!
  @file TM1638liteI2C.cpp
  @brief Part of the I2Cwrapper firmware/library
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#include <TM1638liteI2C.h>


// Constructor
TM1638liteI2C::TM1638liteI2C(I2Cwrapper* w)
{
  wrapper = w;
}


void TM1638liteI2C::attach(uint8_t strobe, uint8_t clock, uint8_t data) {
  wrapper->prepareCommand(TM1638liteAttachCmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->buf.write(strobe);
  wrapper->buf.write(clock);
  wrapper->buf.write(data);
  wrapper->sendCommand();
  if (wrapper->sendCommand() and wrapper->readResult(TM1638liteAttachResult)) {
    wrapper->buf.read(myNum); 
  } // no else clause, as myNum was initialized with -1 (= error)
}

void TM1638liteI2C::sendCommand(uint8_t value) {
  wrapper->prepareCommand(TM1638liteSendCommandCmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->buf.write(value);
  wrapper->sendCommand();
}

void TM1638liteI2C::reset() {
  wrapper->prepareCommand(TM1638liteResetCmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->sendCommand();
}

uint8_t TM1638liteI2C::readButtons() {
  wrapper->prepareCommand(TM1638liteReadButtonsCmd, myNum);
  uint8_t res;
  if (wrapper->sendCommand() and wrapper->readResult(TM1638liteReadButtonsResult)) {
    wrapper->buf.read(res);
  }
  return res;
}

void TM1638liteI2C::setLED(uint8_t position, uint8_t value) {
  wrapper->prepareCommand(TM1638liteSetLEDCmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->buf.write(position);
  wrapper->buf.write(value);
  wrapper->sendCommand();
}

// implemented locally, so we don't have to deal with arbitrary long strings
// Code of this function taken from the original TM1638lite library (c) by Danny Ayers
void TM1638liteI2C::displayText(String text) {  
  uint8_t length = text.length();
  for (uint8_t i = 0; i < length; i++) {
    for (uint8_t position = 0; position < 8; position++) {
      displayASCII(position, text[position]);
    }
  }
}

void TM1638liteI2C::displaySS(uint8_t position, uint8_t value) {
  wrapper->prepareCommand(TM1638liteDisplaySSCmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->buf.write(position);
  wrapper->buf.write(value);
  wrapper->sendCommand();
}

void TM1638liteI2C::displayASCII(uint8_t position, uint8_t ascii) {
  wrapper->prepareCommand(TM1638liteDisplayASCIICmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->buf.write(position);
  wrapper->buf.write(ascii);
  wrapper->sendCommand();
}

void TM1638liteI2C::displayHex(uint8_t position, uint8_t hex) {
  wrapper->prepareCommand(TM1638liteDisplayHexCmd, myNum); // uses units (myNum is a member variable of TM1638liteI2C)
  wrapper->buf.write(position);
  wrapper->buf.write(hex);
  wrapper->sendCommand();
}

