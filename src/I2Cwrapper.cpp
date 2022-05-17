/*!
 *  @file I2Cwrapper.cpp
 *  @brief Part of the I2Cwrapper firmware/library
 *  @section author Author
 *  Copyright (c) 2022 juh
 *  @section license License
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2.
 */


#include <Wire.h>
#include "I2Cwrapper.h"


// Constructor
I2Cwrapper::I2Cwrapper(uint8_t i2c_address, uint8_t maxBuf)
{
  address = i2c_address;
  buf.init(maxBuf);
}


// Jan's little sister: wait I2Cdelay, adjusted by the time already spent
void I2Cwrapper::doDelay()
{
  unsigned long del = I2Cdelay - (millis() - lastI2Ctransmission);
  if (del <= I2Cdelay) { // don't wait if overflow = delay has already passed
    delay(del);
  }
  lastI2Ctransmission = millis();
}

// reset buffer and write header bytes...
void I2Cwrapper::prepareCommand(uint8_t cmd, uint8_t unit)
{
  buf.reset();
  buf.write(cmd);     // [1]: command
  buf.write(unit);    // [2]: subunit to be addressed
  log("    Sending command #"); log(cmd);
  log(" to unit #"); log(int8_t(unit));
}

// ... compute checksum and send it.
// returns true if sending was successful.
// Also updates sentOK and sentErrors for client to check
bool I2Cwrapper::sendCommand()
{
  doDelay(); // give target time in between transmissions
  buf.setCRC8();  // [0]: CRC8
  Wire.beginTransmission(address);
  Wire.write(buf.buffer, buf.idx);
#if defined(DEBUG)
  log(" with CRC="); log(buf.buffer[0]); log(" and ");
  log(buf.idx - 3); log(" paramter bytes: ");
  for (uint8_t d = 3; d < buf.idx; d++) {
    log(buf.buffer[d]); log(" ");
  }
  log("\n");
#endif
  sentOK = (Wire.endTransmission() == 0);
  if (!sentOK) {
    sentErrorsCount++;
  }
  return sentOK;
}

// read target's reply, numBytes is *without* CRC8 byte
// returns true if received data was correct regarding expected lenght and checksum
// Also updates resultOK and resultErrors for client to check
bool I2Cwrapper::readResult(uint8_t numBytes)
{
  doDelay(); // give target time in between transmissions
  buf.reset();
 
  if (Wire.requestFrom(address, uint8_t(numBytes + 1)) > 0) { // +1 for CRC8
    log("    Requesting result ("); log(numBytes + 1); log(" bytes incl. CRC8): ");
    uint8_t i = 0;
    while ((i <= numBytes) and (i < buf.maxLen)) {
      buf.buffer[i++] = Wire.read(); // accessing buffer directly to put CRC8 where it belongs, ### improve
      log(buf.buffer[i - 1], HEX);
      log(" ");
    }
    buf.idx = i;
    resultOK = buf.checkCRC8();
    log((i <= numBytes) ? " -- buffer out of space!  " : "");
    log(" total bytes = ");
    log(buf.idx);
    log(resultOK ? "  CRC8 ok\n" : "  CRC8 wrong!\n");
  } // else some transmission error occured, resultOK = false

  buf.reset(); // reset for reading
  if (!resultOK) {
    resultErrorsCount++;
  }
  return resultOK;
}


bool I2Cwrapper::ping()
{
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void I2Cwrapper::reset()
{
  prepareCommand(resetCmd);
  sendCommand();
}

void I2Cwrapper::changeI2Caddress(uint8_t newAddress)
{
  prepareCommand(changeI2CaddressCmd);
  buf.write(newAddress);
  sendCommand();
}

unsigned long I2Cwrapper::setI2Cdelay(unsigned long delay)
{
  unsigned long d = I2Cdelay;
  I2Cdelay = delay;
  return d;
}

void I2Cwrapper::setInterruptPin(int8_t pin,
                                 bool activeHigh)
{
  prepareCommand(setInterruptPinCmd);
  buf.write(pin);
  buf.write(activeHigh);
  sendCommand();
}

uint8_t I2Cwrapper::clearInterrupt()
{
  prepareCommand(clearInterruptCmd);
  uint8_t res = 0xff;
  if (sendCommand() and readResult(clearInterruptResult)) {
    buf.read(res);
  }
  return res;
}

uint32_t I2Cwrapper::getVersion()
{
  prepareCommand(getVersionCmd);
  uint32_t res = 0xffffffff;
  if (sendCommand() and readResult(getVersionResult)) {
    buf.read(res);
  }
  return res;
}

bool I2Cwrapper::checkVersion(uint32_t controllerVersion)
{
  return controllerVersion == getVersion();
}

uint16_t I2Cwrapper::sentErrors()
{
  uint16_t se = sentErrorsCount;
  sentErrorsCount = 0;
  return se;
}

uint16_t I2Cwrapper::resultErrors()
{
  uint16_t re = resultErrorsCount;
  resultErrorsCount = 0;
  return re;
}

uint16_t I2Cwrapper::transmissionErrors()
{
  return sentErrors() + resultErrors();
}
