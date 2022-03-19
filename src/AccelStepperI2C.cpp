/*!
  @file AccelStepperI2C.cpp
  @brief Part of the AccelStepperI2C firmware/library
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/

#include <Wire.h>
#include <AccelStepperI2C.h>


// Constructor
AccelStepperI2C::AccelStepperI2C(I2Cwrapper* w)
{
  wrapper = w;
}

void AccelStepperI2C::attach(uint8_t interface,
                             uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4,
                             bool enable)
{
  wrapper->prepareCommand(attachCmd);
  wrapper->buf.write(interface);
  wrapper->buf.write(pin1);
  wrapper->buf.write(pin2);
  wrapper->buf.write(pin3);
  wrapper->buf.write(pin4);
  wrapper->buf.write(enable);
  if (wrapper->sendCommand() and wrapper->readResult(attachResult)) {
    wrapper->buf.read(myNum);
  } // else leave myNum at -1 (= failed)
  log("Stepper attached with myNum="); log(myNum); log("\n");
}


void AccelStepperI2C::moveTo(long absolute)
{
  wrapper->prepareCommand(moveToCmd, myNum);
  wrapper->buf.write(absolute);
  wrapper->sendCommand();
}


void AccelStepperI2C::move(long relative)
{
  wrapper->prepareCommand(moveCmd, myNum);
  wrapper->buf.write(relative);
  wrapper->sendCommand();
}


// don't use this, use state machine instead
boolean AccelStepperI2C::run()
{
  wrapper->prepareCommand(runCmd, myNum);
  boolean res = false; // will be returned on transmission error
  if (wrapper->sendCommand() and wrapper->readResult(runResult)) {
    wrapper->buf.read(res); // else return result of function call
  }
  return res;
}


// don't use this, use state machine instead
boolean AccelStepperI2C::runSpeed()
{
  wrapper->prepareCommand(runSpeedCmd, myNum);
  boolean res = false; // will be returned on transmission error
  if (wrapper->sendCommand() and wrapper->readResult(runSpeedResult)) {
    wrapper->buf.read(res); // else return result of function call
  }
  return res;
}


// don't use this, use state machine instead
bool AccelStepperI2C::runSpeedToPosition()
{
  wrapper->prepareCommand(runSpeedToPositionCmd, myNum);
  boolean res = false; // will be returned on transmission error
  if (wrapper->sendCommand() and wrapper->readResult(runSpeedToPositionResult)) {
    wrapper->buf.read(res); // else return result of function call
  }
  return res;
}


long AccelStepperI2C::distanceToGo()
{
  wrapper->prepareCommand(distanceToGoCmd, myNum);
  long res = resError; // funny value returned on error
  if (wrapper->sendCommand() and wrapper->readResult(distanceToGoResult)) {
    wrapper->buf.read(res);  // else return result of function call
  }
  return res;
}


long AccelStepperI2C::targetPosition()
{
  wrapper->prepareCommand(targetPositionCmd, myNum);
  long res = resError; // funny value returned on error
  if (wrapper->sendCommand() and wrapper->readResult(targetPositionResult)) {
    wrapper->buf.read(res);  // else return result of function call
  }
  return res;
}


long AccelStepperI2C::currentPosition()
{
  wrapper->prepareCommand(currentPositionCmd, myNum);
  long res = resError; // funny value returned on error
  if (wrapper->sendCommand() and wrapper->readResult(currentPositionResult)) {
    wrapper->buf.read(res);  // else return result of function call
  }
  return res;
}


void AccelStepperI2C::setCurrentPosition(long position)
{
  wrapper->prepareCommand(setCurrentPositionCmd, myNum);
  wrapper->buf.write(position);
  wrapper->sendCommand();
}


void AccelStepperI2C::setMaxSpeed(float speed)
{
  wrapper->prepareCommand(setMaxSpeedCmd, myNum);
  wrapper->buf.write(speed);
  wrapper->sendCommand();
}


float AccelStepperI2C::maxSpeed()
{
  wrapper->prepareCommand(maxSpeedCmd, myNum);
  float res = resError; // funny value returned on error
  if (wrapper->sendCommand() and wrapper->readResult(maxSpeedResult)) {
    wrapper->buf.read(res);
  }
  return res;
}


void AccelStepperI2C::setAcceleration(float acceleration)
{
  wrapper->prepareCommand(setAccelerationCmd, myNum);
  wrapper->buf.write(acceleration);
  wrapper->sendCommand();
}


void AccelStepperI2C::setSpeed(float speed)
{
  wrapper->prepareCommand(setSpeedCmd, myNum);
  wrapper->buf.write(speed);
  wrapper->sendCommand();
}


float AccelStepperI2C::speed()
{
  wrapper->prepareCommand(speedCmd, myNum);
  float res = resError; // funny value returned on error
  if (wrapper->sendCommand() and wrapper->readResult(speedResult)) {
    wrapper->buf.read(res);
  }
  return res;
}


void    AccelStepperI2C::disableOutputs()
{
  wrapper->prepareCommand(disableOutputsCmd, myNum);
  wrapper->sendCommand();
}


void    AccelStepperI2C::enableOutputs()
{
  wrapper->prepareCommand(enableOutputsCmd, myNum);
  wrapper->sendCommand();
}


void AccelStepperI2C::setMinPulseWidth(unsigned int minWidth)
{
  wrapper->prepareCommand(setMinPulseWidthCmd, myNum);
  wrapper->buf.write(minWidth);
  wrapper->sendCommand();
}


void AccelStepperI2C::setEnablePin(uint8_t enablePin)
{
  wrapper->prepareCommand(setEnablePinCmd, myNum);
  wrapper->buf.write(enablePin);
  wrapper->sendCommand();
}


void AccelStepperI2C::setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert)
{
  wrapper->prepareCommand(setPinsInverted1Cmd, myNum);
  uint8_t bits = (uint8_t)directionInvert << 0 | (uint8_t)stepInvert << 1 | (uint8_t)enableInvert << 2;
  wrapper->buf.write(bits);
  wrapper->sendCommand();
}


void AccelStepperI2C::setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert)
{
  wrapper->prepareCommand(setPinsInverted2Cmd, myNum);
  uint8_t bits = (uint8_t)pin1Invert << 0 | (uint8_t)pin2Invert << 1 | (uint8_t)pin3Invert << 2 | (uint8_t)pin4Invert << 3 | (uint8_t)enableInvert << 4;
  wrapper->buf.write(bits);
  wrapper->sendCommand();
}

// blocking, implemented with state machine
void AccelStepperI2C::runToPosition()
{
  runState(); // start state machine with currently set target
  while (isRunning()) {
    delay(100); // this is a bit arbitrary, but given usual motor applications, a precision of 1/10 second should be ok in most situations
  }
}

// blocking
void AccelStepperI2C::runToNewPosition(long position)
{
  moveTo(position);
  runToPosition();
}


void AccelStepperI2C::stop()
{
  wrapper->prepareCommand(stopCmd, myNum);
  wrapper->sendCommand();
}


bool AccelStepperI2C::isRunning()
{
  wrapper->prepareCommand(isRunningCmd, myNum);
  bool res = false;
  if (wrapper->sendCommand() and wrapper->readResult(isRunningResult)) {
    wrapper->buf.read(res);
  }
  return res;
}



/*
 *
 * Methods specific to AccelStepperI2C start here
 *
 */

void AccelStepperI2C::enableDiagnostics(bool enable)
{
  wrapper->prepareCommand(enableDiagnosticsCmd, myNum);
  wrapper->buf.write(enable);
  wrapper->sendCommand();
}


void AccelStepperI2C::diagnostics(diagnosticsReport* report)
{
  wrapper->prepareCommand(diagnosticsCmd, myNum);
  if (wrapper->sendCommand() and wrapper->readResult(diagnosticsResult)) {
    wrapper->buf.read(*report); // dereference. I *love* how many uses c++ found for the asterisk...
  }
}


void AccelStepperI2C::setState(uint8_t newState)
{
  wrapper->prepareCommand(setStateCmd, myNum);
  wrapper->buf.write(newState);
  wrapper->sendCommand();
}


uint8_t AccelStepperI2C::getState()
{
  wrapper->prepareCommand(getStateCmd, myNum);
  uint8_t state = -1;
  if (wrapper->sendCommand() and wrapper->readResult(getStateResult)) {
    wrapper->buf.read(state);
  }
  return state;
}


void AccelStepperI2C::stopState()
{
  setState(state_stopped);
}


void AccelStepperI2C::runState()
{
  setState(state_run);
}


void AccelStepperI2C::runSpeedState()
{
  setState(state_runSpeed);
}


void AccelStepperI2C::runSpeedToPositionState()
{
  setState(state_runSpeedToPosition);
}


void AccelStepperI2C::setEndstopPin(int8_t pin,
                                    bool activeLow,
                                    bool internalPullup)
{
  wrapper->prepareCommand(setEndstopPinCmd, myNum);
  wrapper->buf.write(pin);
  wrapper->buf.write(activeLow);
  wrapper->buf.write(internalPullup);
  wrapper->sendCommand();
}


void AccelStepperI2C::enableEndstops(bool enable)
{
  wrapper->prepareCommand(enableEndstopsCmd, myNum);
  wrapper->buf.write(enable);
  wrapper->sendCommand();
}


uint8_t AccelStepperI2C::endstops()   // returns endstop(s) states in bits 0 and 1
{
  wrapper->prepareCommand(endstopsCmd, myNum);
  uint8_t res = 0xff; // send "all endstops hit" on transmission error
  if (wrapper->sendCommand() and wrapper->readResult(endstopsResult)) {
    wrapper->buf.read(res); // else send real result
  }
  return res;
}


void AccelStepperI2C::enableInterrupts(bool enable)
{
  wrapper->prepareCommand(enableInterruptsCmd, myNum);
  wrapper->buf.write(enable);
  wrapper->sendCommand();
}

