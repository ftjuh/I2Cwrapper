/*!
   @file SimpleBuffer.h
   @brief Simple and ugly serialization buffer for any data type.
   Template technique and CRC8 adapted from Nick Gammon.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/

#include <SimpleBuffer.h>


void SimpleBuffer::init(uint8_t buflen)
{
  buffer = new uint8_t [buflen];
  maxLen = buflen;
  idx = 1; // first usable position, [0] is for crc8
}

void SimpleBuffer::reset()
{
  idx = 1;
}

// calculate 8-bit CRC (seems to be CRC-8/MAXIM)
// adapted from Nick Gammon's page at http://www.gammon.com.au/i2c
uint8_t SimpleBuffer::calculateCRC8 ()
{
  uint8_t* addr = &buffer[1];
  uint8_t len = idx - 1;
  uint8_t  crc = 0;
  while (len-- > 0) {
    uint8_t  inbyte = *addr++;
    for (uint8_t  i = 8; i; i--) {
      uint8_t  mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix)
      { crc ^= 0x8C; }
      inbyte >>= 1;
    }  // end of for
  }  // end of while
  log("[sb_CRC8="); log(crc); log("] ");
  return crc;
}  // end of crc8

void SimpleBuffer::setCRC8()
{
  buffer[0] = calculateCRC8();
}

bool SimpleBuffer::checkCRC8()
{
  return buffer[0] == calculateCRC8();
}
