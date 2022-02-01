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

//#define DEBUG_AccelStepperI2C

#ifdef DEBUG_AccelStepperI2C
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif


/*
 *
 * Classless helper functions start here
 * ### use a common function for sending, ideally as method of an object 
 * representing the slave as a whole
 *
 */

// Tell slave to reset.
// Not a method, it needs to work even before any steppers have been constructed.
void resetAccelStepperSlave(uint8_t address) {
  SimpleBuffer b; b.init(4);
  b.write(resetCmd); b.write((uint8_t)-1); // placeholder for stepper address, not used here
  b.setCRC8();
  Wire.beginTransmission(address);
  Wire.write(b.buffer, b.idx);
  Wire.endTransmission();
}

// Tell slave at address to permanently change address to newAddress
void changeI2Caddress(uint8_t address, uint8_t newAddress) {
  SimpleBuffer b; b.init(5);
  b.write(changeI2CaddressCmd); b.write((uint8_t)-1); // placeholder for stepper address, not used here
  b.write(newAddress);
  b.setCRC8();
  Wire.beginTransmission(address);
  Wire.write(b.buffer, b.idx);
  Wire.endTransmission();
}


/*
 * 
 * Helper methods start here
 *
 */

// reset stepper's buffer and write header bytes...
void AccelStepperI2C::prepareCommand(uint8_t cmd) {
  buf.reset();
  buf.write(cmd);     // [1]: command
  buf.write(myNum);   // [2]: number of stepper to address
}

// ... compute checksum and send it.
// returns true if sending was successful. Also sets sentOK for client to check
bool AccelStepperI2C::sendCommand() {
  delay(I2CrequestDelay); // give slave time in between commands ### needs tuning or better make it configurable, as it will depend on µC and bus frequency etc.
  buf.setCRC8();  // [0]: CRC8
  Wire.beginTransmission(address);
  Wire.write(buf.buffer, buf.idx);
  sentOK = (Wire.endTransmission() == 0);  // for client to check if command was transmitted successfully
  log("Command '"); log(buf.buffer[1]); log("' sent to stepper #"); log(buf.buffer[2]); 
  log(" with CRC="); log(buf.buffer[0]); log(" and "); log(buf.idx-3); log(" paramter bytes\n");
  return sentOK; // internal: true on success;
}


// read slave's reply, numBytes is *without* CRC8 byte
// returns true if received data was correct regarding expected lenght and checksum
bool AccelStepperI2C::readResult(uint8_t numBytes) {

  delay(I2CrequestDelay); // give slave time to answer ### needs tuning or better make it configurable, as it will depend on µC and bus frequency etc.
  buf.reset();

  if (Wire.requestFrom(address, uint8_t(numBytes + 1)) > 0) { // +1 for CRC8
    log("Requesting result ("); log(numBytes + 1); log(" bytes incl. CRC8): ");
    uint8_t i = 0;
    while ((i <= numBytes) and (i < buf.maxLen)) {
      buf.buffer[i++] = Wire.read(); // accessing buffer directly to put CRC8 where it belongs, ### improve
      log(buf.buffer[i-1], HEX); log(" ");
    }
    buf.idx = i;
    resultOK = buf.checkCRC8();
    log((i<=numBytes) ? " -- buffer out of space!  " : "");  log(" total bytes = "); log(buf.idx);log(resultOK ? "  CRC8 ok\n" : "  CRC8 wrong!\n");

    
  } else { // some transmission error
    log("-- Failed to transmit\n");
    resultOK = false;
  }

  buf.reset(); // reset for reading
  return resultOK;
  
}



/*
 *
 * AccelStepper wrapper functions start here
 *
 */

// Constructor
AccelStepperI2C::AccelStepperI2C(uint8_t i2c_address,
                                 uint8_t interface,
                                 uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4,
                                 bool enable) {
  address = i2c_address;
  buf.init(maxBuf);
  prepareCommand(addStepperCmd); // Will send -1 as stepper num, since myNum is not set yet, but that's ok, as addStepperCmd doesn't need it.
  buf.write(interface); // parameters
  buf.write(pin1);
  buf.write(pin2);
  buf.write(pin3);
  buf.write(pin4);
  buf.write(enable);
  if (sendCommand() and readResult(addStepperResult)) {
    buf.read(myNum);
  } // else leave myNum at -1 (= failed)
}


void AccelStepperI2C::moveTo(long absolute) {
  prepareCommand(moveToCmd);
  buf.write(absolute);
  sendCommand();
}


void AccelStepperI2C::move(long relative) {
  prepareCommand(moveCmd);
  buf.write(relative);
  sendCommand();
}


// don't use this, use state machine instead
// If you use it, be sure to also check sentOK and, if using the result, resultOK.
boolean AccelStepperI2C::run() {
  prepareCommand(runCmd);
  boolean res = false; // will be returned on transmission error
  if (sendCommand() and readResult(runResult)) {
    buf.read(res); // else return result of function call
  } 
  return res;
}


// don't use this, use state machine instead
// If you use it, be sure to also check sentOK and, if using the result, resultOK.
boolean AccelStepperI2C::runSpeed() {
  prepareCommand(runSpeedCmd);
  boolean res = false; // will be returned on transmission error
  if (sendCommand() and readResult(runSpeedResult)) {
    buf.read(res); // else return result of function call
  } 
  return res;
}


// don't use this, use state machine instead
// If you use it, be sure to also check sentOK and, if using the result, resultOK.
bool AccelStepperI2C::runSpeedToPosition() {
  prepareCommand(runSpeedToPositionCmd);
  boolean res = false; // will be returned on transmission error
  if (sendCommand() and readResult(runSpeedToPositionResult)) {
    buf.read(res); // else return result of function call
  } 
  return res;
}


long AccelStepperI2C::distanceToGo() {
  prepareCommand(distanceToGoCmd);
  long res = resError; // funny value returned on error
  if (sendCommand() and readResult(distanceToGoResult)) {
    buf.read(res);  // else return result of function call
  }
  return res;
}


long AccelStepperI2C::targetPosition() {
  prepareCommand(targetPositionCmd);
  long res = resError; // funny value returned on error
  if (sendCommand() and readResult(targetPositionResult)) {
    buf.read(res);  // else return result of function call
  }
  return res;
}


long AccelStepperI2C::currentPosition() {
  prepareCommand(currentPositionCmd);
  long res = resError; // funny value returned on error
  if (sendCommand() and readResult(currentPositionResult)) {
    buf.read(res);  // else return result of function call
  }
  return res;
}


void AccelStepperI2C::setCurrentPosition(long position) {
  prepareCommand(setCurrentPositionCmd);
  buf.write(position);
  sendCommand();
}


void AccelStepperI2C::setMaxSpeed(float speed) {
  prepareCommand(setMaxSpeedCmd);
  buf.write(speed);
  sendCommand();
}


float AccelStepperI2C::maxSpeed() {
  prepareCommand(maxSpeedCmd);
  float res = resError; // funny value returned on error 
  if (sendCommand() and readResult(maxSpeedResult)) {
    buf.read(res);
  }
  return res;
}


void AccelStepperI2C::setAcceleration(float acceleration) {
  prepareCommand(setAccelerationCmd);
  buf.write(acceleration);
  sendCommand();
}


void AccelStepperI2C::setSpeed(float speed) {
  prepareCommand(setSpeedCmd);
  buf.write(speed);
  sendCommand();
}


float AccelStepperI2C::speed() {
  prepareCommand(speedCmd);
  float res = resError; // funny value returned on error 
  if (sendCommand() and readResult(speedResult)) {
    buf.read(res);
  }
  return res;
}


void    AccelStepperI2C::disableOutputs() {
  prepareCommand(disableOutputsCmd);
  sendCommand();
}


void    AccelStepperI2C::enableOutputs() {
  prepareCommand(enableOutputsCmd);
  sendCommand();
}


void AccelStepperI2C::setMinPulseWidth(unsigned int minWidth) {
  prepareCommand(setMinPulseWidthCmd);
  buf.write(minWidth);
  sendCommand();
}


void AccelStepperI2C::setEnablePin(uint8_t enablePin) {
  prepareCommand(setEnablePinCmd);
  buf.write(enablePin);
  sendCommand();
}


void AccelStepperI2C::setPinsInverted(bool directionInvert, bool stepInvert, bool enableInvert) {
  prepareCommand(setPinsInverted1Cmd);
  uint8_t bits = (uint8_t)directionInvert << 0 | (uint8_t)stepInvert << 1 | (uint8_t)enableInvert << 2;
  buf.write(bits);
  sendCommand();
}


void AccelStepperI2C::setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert) {
  prepareCommand(setPinsInverted2Cmd);
  uint8_t bits = (uint8_t)pin1Invert << 0 | (uint8_t)pin2Invert << 1 | (uint8_t)pin3Invert << 2 | (uint8_t)pin4Invert << 3 | (uint8_t)enableInvert << 4;
  buf.write(bits);
  sendCommand();
}

// // blocking, not implemented (yet?)
// void AccelStepperI2C::runToPosition() {
// }
// 
// // blocking, not implemented (yet?)
// void AccelStepperI2C::runToNewPosition(long position) {
//   //position++; // just to stop the compiler from complaining
// }

void AccelStepperI2C::stop() {
  prepareCommand(stopCmd);
  sendCommand();
}


bool AccelStepperI2C::isRunning() {
  prepareCommand(isRunningCmd);
  bool res = false;
  if (sendCommand() and readResult(isRunningResult)) {
    buf.read(res);
  }
  return res;
}



/*
 *
 * Methods specific to AccelStepperI2C start here
 *
 */

void AccelStepperI2C::setState(uint8_t newState) {
  prepareCommand(setStateCmd);
  buf.write(newState);
  sendCommand();
}


uint8_t AccelStepperI2C::getState() {
  prepareCommand(getStateCmd);
  uint8_t state = -1;
  if (sendCommand() and readResult(getStateResult)) {
    buf.read(state);
  }
  return state;
}


void AccelStepperI2C::stopState() {
  setState(state_stopped);
}


void AccelStepperI2C::runState() {
  setState(state_run);
}


void AccelStepperI2C::runSpeedState() {
  setState(state_runSpeed);
}


void AccelStepperI2C::runSpeedToPositionState() {
  setState(state_runSpeedToPosition);
}


void AccelStepperI2C::setEndstopPin(int8_t pin,
                                    bool activeLow,
                                    bool internalPullup) {
  prepareCommand(setEndstopPinCmd);
  buf.write(pin);
  buf.write(activeLow);
  buf.write(internalPullup);
  sendCommand();
}


void AccelStepperI2C::enableEndstops(bool enable) {
  prepareCommand(enableEndstopsCmd);
  buf.write(enable);
  sendCommand();
}


uint8_t AccelStepperI2C::endstops() { // returns endstop(s) states in bits 0 and 1
  prepareCommand(endstopsCmd);
  uint8_t res = 0xff; // send "all endstops hit" on transmission error
  if (sendCommand() and readResult(endstopsResult)) {
    buf.read(res); // else send real result
  }
  return res;
}


void AccelStepperI2C::enableDiagnostics(bool enable) {
  prepareCommand(enableDiagnosticsCmd);
  buf.write(enable);
  sendCommand();
}


void AccelStepperI2C::diagnostics(diagnosticsReport* report) {
  prepareCommand(diagnosticsCmd);
  if (sendCommand() and readResult(diagnosticsResult)) {
    buf.read(*report); // dereference. I *love* how many uses c++ found for the asterisk...
  }
}


void AccelStepperI2C::setInterruptPin(int8_t pin, 
                     bool activeHigh) {
  prepareCommand(setInterruptPinCmd);
  buf.write(pin);
  buf.write(activeHigh);
  sendCommand();
  
}

void AccelStepperI2C::enableInterrupts(bool enable) {
  prepareCommand(enableInterruptsCmd);
  buf.write(enable);
  sendCommand();
}
