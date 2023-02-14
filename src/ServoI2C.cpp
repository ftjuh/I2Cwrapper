/*!
  @file ServoI2C.cpp
  @brief Part of the I2Cwrapper firmware/library
  ## Author
  Copyright (c) 2022 juh
  ## License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/

#include <Wire.h>
#include <ServoI2C.h>


// Constructor
ServoI2C::ServoI2C(I2Cwrapper* w)
{
  wrapper = w;
}


uint8_t ServoI2C::attach(int pin)
{
  wrapper->prepareCommand(servoAttach1Cmd);
  wrapper->buf.write((int16_t)pin);
  if (wrapper->sendCommand() and wrapper->readResult(servoAttachResult)) {
    wrapper->buf.read(myNum);
  } // else leave myNum at -1 (= failed)
  log("Servo attached with myNum="); log(myNum); log("\n");
  return (uint8_t)myNum;
}

uint8_t ServoI2C::attach(int pin, int min, int max)
{
  wrapper->prepareCommand(servoAttach2Cmd);
  wrapper->buf.write((int16_t)pin);
  wrapper->buf.write((int16_t)min);
  wrapper->buf.write((int16_t)max);
  if (wrapper->sendCommand() and wrapper->readResult(servoAttachResult)) {
    wrapper->buf.read(myNum);
  } // else leave myNum at -1 (= failed)
  log("Servo attached with myNum="); log(myNum); log("\n");
  return (uint8_t)myNum;
}

void ServoI2C::detach()
{
  wrapper->prepareCommand(servoDetachCmd, myNum);
  wrapper->sendCommand();
}

void ServoI2C::write(int value)
{
  wrapper->prepareCommand(servoWriteCmd, myNum);
  wrapper->buf.write((int16_t)value);
  wrapper->sendCommand();
}

void ServoI2C::writeMicroseconds(int value)
{
  wrapper->prepareCommand(servoWriteMicrosecondsCmd, myNum);
  wrapper->buf.write((int16_t)value);
  wrapper->sendCommand();
}

int ServoI2C::read()
{
  wrapper->prepareCommand(servoReadCmd, myNum);
  int16_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(servoReadResult)) {
    wrapper->buf.read(res);
  }
  return (int)res;
}

int ServoI2C::readMicroseconds()
{
  wrapper->prepareCommand(servoReadMicrosecondsCmd, myNum);
  int16_t res = -1;
  if (wrapper->sendCommand() and wrapper->readResult(servoReadMicrosecondsResult)) {
    wrapper->buf.read(res);
  }
  return (int)res;
}

bool ServoI2C::attached()
{
  wrapper->prepareCommand(servoAttachedCmd, myNum);
  uint8_t res = false;
  if (wrapper->sendCommand() and wrapper->readResult(servoAttachedResult)) {
    wrapper->buf.read(res);
  }
  return (bool)res;
}

