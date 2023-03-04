/*!
  @file RotaryEncoderI2C.cpp
  @brief 
  
  ## Author
  Copyright (c) 2023 juh
  ## License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#include <RotaryEncoderI2C.h>



// Constructor
RotaryEncoderI2C::RotaryEncoderI2C(I2Cwrapper* w)
{
  wrapper = w;
}

/*
 * A note on units:
 * Some modules will want to differentiate between multiple instances of some 
 * hardware like steppers or servos. We call these units. Others like PinI2C 
 * and ESP32sensors don't use units, as there is only one instance of hardware 
 * of this type per target present.
 * 
 * Modules which use units, will need some attach() or similar function to 
 * create a new instance. It will have a myNum variable representing the unit
 * number, which will need to be passed to the target to adress the relevant unit.
 * See ServoI2C and AccelStepperI2C for examples.
 * 
 * Modules which don't use units, like PinI2C, can simply omit the unit parameter 
 * in the below prepareCommand() calls.
 */


int8_t RotaryEncoderI2C::attach(int pin1, int pin2, RotaryEncoder::LatchMode mode) {
  wrapper->prepareCommand(rotaryEncoderAttachCmd);
  wrapper->buf.write((int16_t)pin1);
  wrapper->buf.write((int16_t)pin2);
  wrapper->buf.write((uint8_t)mode);
  if (wrapper->sendCommand() and wrapper->readResult(rotaryEncoderAttachCmdResult)) {
    wrapper->buf.read(myNum);
  } // else leave myNum at -1 (= failed)
  log("RotaryEncoder attached with myNum="); log(myNum); log("\n");
  return myNum;
}

long RotaryEncoderI2C::getPosition() {
  wrapper->prepareCommand(rotaryEncoderGetPositionCmd, myNum);
  int32_t res = 0;
  if (wrapper->sendCommand() and wrapper->readResult(rotaryEncoderGetPositionCmdResult)) {
    wrapper->buf.read(res);
  }
  return (long)res;
}

RotaryEncoder::Direction RotaryEncoderI2C::getDirection() {
  wrapper->prepareCommand(rotaryEncoderGetDirectionCmd, myNum);
  int8_t res = 0;
  if (wrapper->sendCommand() and wrapper->readResult(rotaryEncoderGetDirectionCmdResult)) {
    wrapper->buf.read(res);
  }
  return (RotaryEncoder::Direction)res;
}

void RotaryEncoderI2C::setPosition(long newPosition) {
  wrapper->prepareCommand(rotaryEncoderSetPositionCmd, myNum);
  wrapper->buf.write((uint32_t)newPosition);
  wrapper->sendCommand();
}

unsigned long RotaryEncoderI2C::getMillisBetweenRotations() const {
  wrapper->prepareCommand(rotaryEncoderGetMillisBetweenRotationsCmd, myNum);
  uint32_t res = 0;
  if (wrapper->sendCommand() and 
    wrapper->readResult(rotaryEncoderGetMillisBetweenRotationsCmdResult)) {
    wrapper->buf.read(res);
  }
  return (unsigned long)res;
}

unsigned long RotaryEncoderI2C::getRPM() {
  wrapper->prepareCommand(rotaryEncoderGetRPMCmd, myNum);
  uint32_t res = 0;
  if (wrapper->sendCommand() and wrapper->readResult(rotaryEncoderGetRPMCmdResult)) {
    wrapper->buf.read(res);
  }
  return (unsigned long)res;
}

void RotaryEncoderI2C::startDiagnosticsMode(uint8_t numEncoder) {
  wrapper->prepareCommand(rotaryEncoderStartDiagnosticsModeCmd, myNum);
  wrapper->buf.write(numEncoder);
  wrapper->sendCommand();
}

// diagnostics mode needs no command sent, it will simply send
uint8_t RotaryEncoderI2C::getDiagnostics() {
  uint8_t res = 0xff;
  if (wrapper->readResult(1)) { // diagnostics mode doesn't need a command
    wrapper->buf.read(res);
  }
  return res;
}


