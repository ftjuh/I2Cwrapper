/*!
   @file _addressFromPins_firmware.h

   @brief Makes the target retrieve its own address from the input state of one or
   more pins at startup, so that the end user can change it e.g. with solder
   bridges or DIP switches.
   To adapt for your specific needs, you'll have to change the following values below:
    - I2CaddressOffset - base address when all pins are inactive
    - I2CaddressPins - array of 1 to 7 pins, LSB pin first
    - I2CaddressPinsActiveLow - true for HIGH = 0, LOW = 1
    - I2CaddressPinsPullup - true to use internal pullups for address pins.
      Important: Without internal pullups, you must not leave pins floating, or you'll get a random address!

   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/
/// @cond

/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations

// =========== config ===========

// see above for documentation

const uint8_t I2CaddressOffset = 8;
const uint8_t I2CaddressPins[] = {10 /* bit 0 */ , 11 /* bit 1 */ , 12 /* bit 2 */}; // analog pins will just work the same
const bool I2CaddressPinsActiveLow = true;
const bool I2CaddressPinsPullup = true;

// =========== end of config ===========


uint8_t readAddressFromPins() {
  int addr = 0;
  for (uint8_t i = 0; i < sizeof(I2CaddressPins); i++) {
    if (I2CaddressPinsPullup) {
      pinMode(I2CaddressPins[i], INPUT_PULLUP); // no else clause, as INPUT is the default mode
    }
    addr |= (digitalRead(I2CaddressPins[i])^I2CaddressPinsActiveLow) << i;
  }
  return (I2CaddressOffset + addr);
}

// claim and define address retrieval for this module
#ifndef I2C_ADDRESS_DEFINED_BY_MODULE
#define I2C_ADDRESS_DEFINED_BY_MODULE readAddressFromPins()
#else
#error More than one address defining feature module enabled in firmware_modules.h. Plaese deactivate all but one.
#endif


#endif // MF_STAGE_declarations




/// @endcond
