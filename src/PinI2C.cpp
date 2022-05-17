/*!
  @file PinI2C.cpp
  @brief Part of the I2Cwrapper firmware/library
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#include <PinI2C.h>


const uint8_t myNum = 253; // fake myNum, as there is only one "unit" of the pin interface per target

// Constructor
PinI2C::PinI2C(I2Cwrapper* w)
{
  wrapper = w;
}

void PinI2C::pinMode(uint8_t pin , uint8_t mode) {
  wrapper->prepareCommand(pinPinModeCmd, myNum);
  wrapper->buf.write(pin);
  wrapper->buf.write(mode);
  wrapper->sendCommand();  
}

int PinI2C::digitalRead(uint8_t pin) {
  wrapper->prepareCommand(pinDigitalReadCmd, myNum);
  wrapper->buf.write(pin);
  int16_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(pinDigitalReadResult)) {
    wrapper->buf.read(res);
  }
  return res;  
}

void PinI2C::digitalWrite(uint8_t pin, uint8_t value){
  wrapper->prepareCommand(pinDigitalWriteCmd, myNum);
  wrapper->buf.write(pin);
  wrapper->buf.write(value);
  wrapper->sendCommand();
}

int PinI2C::analogRead(uint8_t pin){
  wrapper->prepareCommand(pinAnalogReadCmd, myNum);
  wrapper->buf.write(pin);
  int16_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(pinDigitalReadResult)) {
    wrapper->buf.read(res);
  }
  return res;  
}

void PinI2C::analogReference(uint8_t mode){
  wrapper->prepareCommand(pinAnalogReferenceCmd, myNum);
  wrapper->buf.write(mode);
  wrapper->sendCommand();
}

void PinI2C::analogWrite(uint8_t pin, int value){
  wrapper->prepareCommand(pinAnalogWriteCmd, myNum);
  wrapper->buf.write(pin);
  wrapper->buf.write((int16_t)value);
  wrapper->sendCommand();
}
