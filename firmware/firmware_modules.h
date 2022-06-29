/*!
   @file firmware_modules.h
   @brief This file determines which modules will be included in the target firmware.
   
   Add other modules or (un)comment out the existing ones as needed.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/


// regular modules

//#include "AccelStepperI2C_firmware.h"   // will not compile on Attinys
//#include "ServoI2C_firmware.h"          // will not compile on Attinys
//#include "PinI2C_firmware.h"            // should work on any platform
//#include "ESP32sensorsI2C_firmware.h"   // will only compile on ESP32 boards
//#include "TM1638liteI2C_firmware.h"     // should work on any platform


// feature modules

#include "_statusLED_firmware.h"        // makes the LED_BUILTIN flash briefly on each received interrupt
