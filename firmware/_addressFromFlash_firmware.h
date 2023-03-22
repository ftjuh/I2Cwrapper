/*!
   @file _addressFromFlash_firmware.h

   @brief Feature module.
   Read target's own I2C address from non volatile memory (EEPROM, flash
   memory) and store a new changed address upon the controller's command.

   ## Author
   Copyright (c) 2022 juh
   ## License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/
/// @cond


/*
     (1) includes
*/

#if MF_STAGE == MF_STAGE_includes
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_STM32)
#define USE_EEPROM
#if defined(ARDUINO_ARCH_SAMD)
#include <FlashAsEEPROM.h>
#else
#include <EEPROM.h>
#endif // SAMD
#endif // AVR ... SAMD
#endif // MF_STAGE_includes


/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations

// =========== config ===========

const int EEPROM_OFFSET_I2C_ADDRESS = 0; // where in eeprom/flash is the I2C address stored, if any? [1 CRC8 + 4 marker + 1 address = 6 bytes]
const uint32_t eepromI2CaddressMarker = 0x12C0ACCF; // arbitrary 32bit marker proving that next byte in EEPROM is in fact an I2C address
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const uint32_t eepromUsedSize = 6; // total bytes of EEPROM used by us
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)

// =========== end of config ===========

/*
   Read a stored I2C address from EEPROM. If there is none, use default.
*/
uint8_t retrieveI2C_address()
{
#ifdef USE_EEPROM
  SimpleBuffer b;
  b.init(8);
  // read 6 bytes from eeprom: [0]=CRC8; [1-4]=marker; [5]=address
  log("Reading I2C address from EEPROM: ");
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.begin(256);
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  for (byte i = 0; i < 6; i++) {
    b.buffer[i] = EEPROM.read(EEPROM_OFFSET_I2C_ADDRESS + i);
    log(b.buffer[i]); log (" ");
  }
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.end();
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("\n");
  // we wrote directly to the buffer, so reader idx is still at 1
  uint32_t markerTest; b.read(markerTest);
  uint8_t storedAddress; b.read(storedAddress); // now idx will be at 6, so that CRC check below will work
  if (b.checkCRC8() and (markerTest == eepromI2CaddressMarker)) {
    return storedAddress;
  } else
#endif // USE_EEPROM
  {
    log("No stored address found, using default\n");
    return I2CwrapperDefaultAddress;
  }
}

/*!
   Write I2C address to EEPROM
   note: ESP eeprom.h works a bit differently from AVRs: https://arduino-esp8266.readthedocs.io/en/3.0.2/libraries.html#eeprom
*/
void storeI2C_address(uint8_t newAddress)
{
#ifdef USE_EEPROM
  SimpleBuffer b;
  b.init(8);
  b.write(eepromI2CaddressMarker);
  b.write(newAddress);
  b.setCRC8();
  log("Writing to EEPROM: ");
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.begin(32);
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  for (byte i = 0; i < 6; i++) { // [0]=CRC8; [1-4]=marker; [5]=address
    EEPROM.write(EEPROM_OFFSET_I2C_ADDRESS + i, b.buffer[i]);
    //EEPROM.put(EEPROM_OFFSET_I2C_ADDRESS + i, b.buffer[i]);
    //delay(120);
    log(b.buffer[i]); log (" ");
  }
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.end(); // end() will also commit()
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("\n");
#else // USE_EEPROM
  log("No EEPROM for this board architecture, storeI2C_address() did nothing.\n");
#endif // USE_EEPROM
}

// claim and define address retrieval for this module
#ifndef I2C_ADDRESS_DEFINED_BY_MODULE
#define I2C_ADDRESS_DEFINED_BY_MODULE retrieveI2C_address()
#else
#error More than one address defining feature module enabled in firmware_modules.h. Plaese deactivate all but one.
#endif

#endif // MF_STAGE_declarations



/*
     (5) processMessage() function
*/

#if MF_STAGE == MF_STAGE_processMessage

case changeI2CaddressCmd: {
  if (i == 1) { // 1 uint8_t
    uint8_t newAddress; bufferIn->read(newAddress);
    log("Storing new Address "); log(newAddress); log("\n");
    storeI2C_address(newAddress);
  }
}
break;

#endif // MF_STAGE_processMessage




/// @endcond
