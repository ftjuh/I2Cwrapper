/*!
  @file template_I2C.cpp
  @brief Template for a user module/controller library for the I2Cwrapper @ref 
  firmware. "xxx" represents the name of your module, e.g. "PinI2C".
  
  For most modules, the core task of this file is to convert the controller's
  function calls into commands and parameters, send them to the target and,
  optionally, receive its response and return it as function result.
  
  ## Author
  Copyright (c) 2022 juh
  ## License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#include <xxxI2C.h>


// Constructor
xxxI2C::xxxI2C(I2Cwrapper* w)
{
  wrapper = w;
}

/*
 * A note on units:
 * Some modules will want to differentiate between multiple instances of some 
 * hardware like steppers or servos. We call these units. Others like PinI2C 
 * and ESP32sensors don't use units, as there is only one instance of hardware 
 * of this type per target present.
 * 
 * Modules which use units, will need some attach() or similar function to 
 * create a new instance. It will have a myNum variable representing the unit
 * number, which will need to be passed to the target to adress the relevant unit.
 * See ServoI2C and AccelStepperI2C for examples.
 * 
 * Modules which don't use units, like PinI2C, can simply omit the unit parameter 
 * in the below prepareCommand() calls.
 */

// example for a void function/command
void xxxI2C::xxxDemo1(uint8_t arg1) {
  
  wrapper->prepareCommand(xxxDemo1Cmd); // doesn't use units
  // wrapper->prepareCommand(xxxDemo1Cmd, myNum); // uses units (myNum is a member variable of xxxI2C)
  wrapper->buf.write(arg1); // pass the arguments for this function/command, takes any type
  wrapper->sendCommand(); // void functions only use sendCommand() and are done.

}

// example for a non void function/command
uint8_t xxxI2C::xxxDemo2(uint16_t arg1, float arg2, bool arg3) {
  
  wrapper->prepareCommand(xxxDemo2Cmd); // doesn't use units
  // wrapper->prepareCommand(xxxDemo2Cmd, myNum); // uses units (myNum is a member variable of xxxI2C)
  wrapper->buf.write(arg1); // pass the arguments for this function/command
  wrapper->buf.write(arg2); // buf.write() accepts any type 
  wrapper->buf.write(arg3); // and will write sizeof(type) bytes to the buffer
  
  // non void functions need to request the target's reply after sending the command
  uint8_t res; // adapt to the return type of this function
  if (wrapper->sendCommand() and wrapper->readResult(xxxDemo2CmdResult)) { // reply is checked for correct length
    wrapper->buf.read(res); // read the result from the buffer
  }
  return res; // and return it to the caller
  
}
