/*!
  @file ESP32sensorsI2C.cpp
  @brief Part of the I2Cwrapper firmware/library
  ## Author
  Copyright (c) 2022 juh
  ## License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/

#include <ESP32sensorsI2C.h>



const uint8_t myNum = 252; // fake myNum, as there is only one "unit" of the pin interface per target

// Constructor
ESP32sensorsI2C::ESP32sensorsI2C(I2Cwrapper* w)
{
  wrapper = w;
}

void ESP32sensorsI2C::touchSetCycles(uint16_t measure, uint16_t sleep) {
  wrapper->prepareCommand(ESP32sensorsTouchSetCyclesCmd, myNum);
  wrapper->buf.write(measure);
  wrapper->buf.write(sleep);
  wrapper->sendCommand();  
}

uint16_t ESP32sensorsI2C::touchRead(uint8_t pin) {
  wrapper->prepareCommand(ESP32sensorsTouchReadCmd, myNum);  
  wrapper->buf.write(pin);
  uint16_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(ESP32sensorsTouchReadResult)) {
    wrapper->buf.read(res);
  }
  return res;  
}

void ESP32sensorsI2C::enableInterrupts(uint8_t pin, uint16_t threshold, bool falling) {
  wrapper->prepareCommand(ESP32sensorsEnableInterruptsCmd, myNum);
  wrapper->buf.write(pin);
  wrapper->buf.write(threshold);
  wrapper->buf.write(falling);
  wrapper->sendCommand();  
}

int ESP32sensorsI2C::hallRead() {
  wrapper->prepareCommand(ESP32sensorsHallReadCmd, myNum);
  int16_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(ESP32sensorsHallReadResult)) {
    wrapper->buf.read(res);
  }
  return res;    
}

float ESP32sensorsI2C::temperatureRead() {
  wrapper->prepareCommand(ESP32sensorsTemperatureReadCmd, myNum);
  float res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(ESP32sensorsTemperatureReadResult)) {
    wrapper->buf.read(res);
  }
  return res;    
}

