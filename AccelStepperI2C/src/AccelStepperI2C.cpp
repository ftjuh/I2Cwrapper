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

// ms to wait between I2C communication, can be changed by setI2Cdelay()
uint16_t I2Cdelay = I2CdefaultDelay;

/*
 * Classless helper functions start here
 */

// reset buffer and write header bytes...
void prepareCommand(SimpleBuffer& buf, uint8_t cmd, uint8_t stepper = -1) {
  buf.reset();
  buf.write(cmd);     // [1]: command
  buf.write(stepper); // [2]: number of stepper to address, defaults to -1 (for general commands)
}

// ... compute checksum and send it.
// returns true if sending was successful.
bool sendCommand(SimpleBuffer& buf, uint8_t address) {
  delay(I2Cdelay); // give slave time in between commands
  buf.setCRC8();  // [0]: CRC8
  Wire.beginTransmission(address);
  Wire.write(buf.buffer, buf.idx);
  log("Command '"); log(buf.buffer[1]);
  log("' sent to stepper #"); log(buf.buffer[2]);
  log(" with CRC="); log(buf.buffer[0]); log(" and "); 
  log(buf.idx-3); log(" paramter bytes\n");
  return (Wire.endTransmission() == 0);
}

// read slave's reply, numBytes is *without* CRC8 byte
// returns true if received data was correct regarding expected lenght and checksum
bool readResult(SimpleBuffer& buf, uint8_t numBytes, uint8_t address) {

  delay(I2Cdelay); // give slave time to answer
  buf.reset();
  bool res;

  if (Wire.requestFrom(address, uint8_t(numBytes + 1)) > 0) { // +1 for CRC8
    log("Requesting result (");
    log(numBytes + 1);
    log(" bytes incl. CRC8): ");
    uint8_t i = 0;
    while ((i <= numBytes) and (i < buf.maxLen)) {
      buf.buffer[i++] = Wire.read(); // accessing buffer directly to put CRC8 where it belongs, ### improve
      log(buf.buffer[i-1], HEX);
      log(" ");
    }
    buf.idx = i;
    res = buf.checkCRC8();
    log((i<=numBytes) ? " -- buffer out of space!  " : "");
    log(" total bytes = ");
    log(buf.idx);
    log(resultOK ? "  CRC8 ok\n" : "  CRC8 wrong!\n");
  } else { // some transmission error
    log("-- Failed to transmit\n");
    res = false;
  }

  buf.reset(); // reset for reading
  return res;

}


/*
 * Classless general command functions start here
 */

// Tell slave to reset.
// Not a method, it needs to work even before any steppers have been constructed.
void resetAccelStepperSlave(uint8_t address) {
  SimpleBuffer b;
  b.init(4);
  prepareCommand(b, resetCmd);
  sendCommand(b, address);
}

// Tell slave to permanently change address to newAddress
void changeI2Caddress(uint8_t address, uint8_t newAddress) {
  SimpleBuffer b;
  b.init(5);
  prepareCommand(b, changeI2CaddressCmd);
  b.write(newAddress);
  sendCommand(b, address);
}

// needs no address, is used on master's side only
uint16_t setI2Cdelay(uint16_t delay) {
  uint16_t d = I2Cdelay;
  I2Cdelay = delay;
  return d;
}

void setInterruptPin(uint8_t address,
                     int8_t pin,
                     bool activeHigh) {
  SimpleBuffer b;
  b.init(6);
  prepareCommand(b, setInterruptPinCmd);
  b.write(pin);
  b.write(activeHigh);
  sendCommand(b, address);
}

uint16_t getVersion(uint8_t address) {
  SimpleBuffer b;
  b.init(4);
  prepareCommand(b, getVersionCmd);
  uint16_t res = 0xffff;
  if (sendCommand(b, address) and readResult(b, getVersionResult, address)) {
    b.read(res);
  }
  return res;
}

bool checkVersion(uint8_t address) {
  uint16_t slaveVersion = getVersion(address);
  uint16_t libraryVersion = AccelStepperI2C_VersionMinor | AccelStepperI2C_VersionMajor << 8;
  return slaveVersion == libraryVersion;
}

void enableDiagnostics(uint8_t address, bool enable) {
  SimpleBuffer b;
  b.init(5);
  prepareCommand(b, enableDiagnosticsCmd);
  b.write(enable);
  sendCommand(b, address);
}


void diagnostics(uint8_t address, 
                 diagnosticsReport* report) {
  SimpleBuffer b;
  b.init(diagnosticsResult + 1); // +1 for CRC8
  prepareCommand(b, diagnosticsCmd);
  if (sendCommand(b, address) and readResult(b, diagnosticsResult, address)) {
    b.read(*report); // dereference. I *love* how many uses c++ found for the asterisk...
  }
}



/*
 *
 * Helper methods for AccelStepperI2C start here
 *
 */

// reset stepper's buffer and write header bytes...
void AccelStepperI2C::prepareCommand(uint8_t cmd) {
  ::prepareCommand(buf, cmd, myNum); // '::' -> use function in global namespace
}

// ... compute checksum and send it.
// returns true if sending was successful.
// Also updates sentOK and sentErrors for client to check
bool AccelStepperI2C::sendCommand() {
  sentOK = ::sendCommand(buf, address); // '::' -> use function in global namespace
  sentErrorsCount += sentOK ? 0 : 1;
  return sentOK;
}

// read slave's reply, numBytes is *without* CRC8 byte
// returns true if received data was correct regarding expected lenght and checksum
// Also updates resultOK and resultErrors for client to check
bool AccelStepperI2C::readResult(uint8_t numBytes) {
  resultOK = ::readResult(buf, numBytes, address); // '::' -> use function in global namespace
  resultErrorsCount += resultOK ? 0 : 1;
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
boolean AccelStepperI2C::run() {
  prepareCommand(runCmd);
  boolean res = false; // will be returned on transmission error
  if (sendCommand() and readResult(runResult)) {
    buf.read(res); // else return result of function call
  }
  return res;
}


// don't use this, use state machine instead
boolean AccelStepperI2C::runSpeed() {
  prepareCommand(runSpeedCmd);
  boolean res = false; // will be returned on transmission error
  if (sendCommand() and readResult(runSpeedResult)) {
    buf.read(res); // else return result of function call
  }
  return res;
}


// don't use this, use state machine instead
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


void AccelStepperI2C::enableInterrupts(bool enable) {
  prepareCommand(enableInterruptsCmd);
  buf.write(enable);
  sendCommand();
}


uint16_t AccelStepperI2C::sentErrors() {
  uint16_t se = sentErrorsCount;
  sentErrorsCount = 0;
  return se;
}

uint16_t AccelStepperI2C::resultErrors() {
  uint16_t re = resultErrorsCount;
  resultErrorsCount = 0;
  return re;
}

uint16_t AccelStepperI2C::transmissionErrors() {
  return sentErrors() + resultErrors();
}
