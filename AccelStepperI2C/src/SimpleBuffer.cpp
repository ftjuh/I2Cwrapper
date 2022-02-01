/*!
   @file SimpleBuffer.h
   @brief simple and ugly serialization buffer for any types.
   Template technique adapted from Nick Gammon's I2C_Anything library. Thanks, Nick!
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/

#include <SimpleBuffer.h>

#ifdef DEBUG_AccelStepperI2C
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif


void SimpleBuffer::init(uint8_t buflen) {
  buffer = new uint8_t [buflen];
  maxLen = buflen;
  idx = 1; // first usable position, [0] is for crc8
}

void SimpleBuffer::reset() {
  idx = 1;
}

// calculate 8-bit CRC
// adapted from Nick Gammon's page at http://www.gammon.com.au/i2c (seems to be CRC-8/MAXIM)
uint8_t SimpleBuffer::calculateCRC8 () {
  uint8_t *addr = &buffer[1];
  uint8_t len = idx - 1;
  uint8_t  crc = 0;
  while (len-- > 0) {
    uint8_t  inbyte = *addr++;
    for (uint8_t  i = 8; i; i--) {
      uint8_t  mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix)
        crc ^= 0x8C;
      inbyte >>= 1;
    }  // end of for
  }  // end of while
  log("--CRC8 = "); log(crc, HEX); log(" --");
  return crc;
}  // end of crc8

void SimpleBuffer::setCRC8() {
  buffer[0] = calculateCRC8();
}

bool SimpleBuffer::checkCRC8() {
  return (buffer[0] == calculateCRC8());
}
