/*!
  @file template_I2C.h
  @brief Template for a user module/controller library for the I2Cwrapper @ref 
  firmware.ino. "xxx" represents the name of your module, e.g. "PinI2C"
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#ifndef xxxI2C_h
#define xxxI2C_h

// #define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your controller's setup()


#include <Arduino.h>
#include <I2Cwrapper.h>


#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log


/*!
 * @brief
 * These constants represent the commands you want youre target device to 
 * understand. They will also be needed by your firmware module (see
 * template_I2C_firmware.h). Usually, each command code will correspond to one 
 * of the library functions below. If you want your target device to use more 
 * than one module, care must be taken that commands are unique for that target. 
 * Currently there is no mechanism in place for controlling that, so it's up to 
 * you to ensure uniqeness of command IDs. See I2Cwrapper.h for currently unused 
 * ID ranges.
 * 
 * Non void functions/commands have an additional uint8_t xxx...Result const
 * next to them. It will be used to check for the correct length of target
 * replies in the related functions' implementation.
 */
// xxx commands
const uint8_t xxxCmdOffset = /* choose an offset that doesn't conflict with other modules */;
const uint8_t xxxDemo2Cmd  = xxxCmdOffset + 1; const uint8_t xxxDemo2CmdResult = 1 /* number of bytes returned for non void functions/commands */;
const uint8_t xxxDemo1Cmd  = xxxCmdOffset + 0; // void functions/commands don't need a "const xxx...Result"
// add more commands here

/*!
  @brief An I2C wrapper class for xxx.
  @details This class ...
  */
class xxxI2C
{
public:
  
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the target the pin is connected to.
   */
  xxxI2C(I2Cwrapper* w);

  void xxxDemo1(uint8_t);
  uint8_t xxxDemo2(uint16_t, float, bool); // return type uint8_t needs to correspond to xxxDemo2CmdResult
  
  int8_t myNum = -1;    // for modules that use 
  
private:
  I2Cwrapper* wrapper;

};


#endif
