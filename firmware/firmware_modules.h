/*!
   @file firmware_modules.h
   @brief This file determines which modules will be included in the target firmware. Add
   other modules or (un)comment out the existing ones as needed.

   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/


/*
  Regular modules
*/

//#include "AccelStepperI2C_firmware.h"   // will not compile on Attinys
//#include "ESP32sensorsI2C_firmware.h"   // will only compile on ESP32 boards
//#include "PinI2C_firmware.h"            // should work on any platform, 
//#include "ServoI2C_firmware.h"          // will not compile on Attinys
//#include "TM1638liteI2C_firmware.h"     // should work on any platform


/*
  Feature modules
*/

#include "_statusLED_firmware.h"        // makes the LED_BUILTIN flash briefly on each received interrupt

// note: use *only one* of the following "_address..." modules at a time.
// If all are deactivated, the default I2CwrapperDefaultAddress (0x8) will be used

//#include "_addressFixed_firmware.h"     // use a (non default) fixed I2C address for the target
//#include "_addressFromPins_firmware.h"  // make the target read its I2C address from a given set of input pin states (jumper bridges etc.)
//#include "_addressFromFlash_firmware.h" // read I2C address from flash memory or EEPROM, also implements the command to change and store a new address
