/*!
  @file PinI2C.h
  @brief Arduino library for I2C-control of digital and analog pins connected to
  another Arduino which runs the associated @ref firmware.ino firmware.
  This is more or less a byproduct of the AccelStepperI2C project, but could be
  used as a (slightly bloated) standalone solution for I2C control of Arduino pins.
  Just as AccelStepperI2C, PinI2C mimicks the interface of the original
  Arduino pin control functions, so that it can be used with only little adaptations.
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#ifndef PinI2C_h
#define PinI2C_h

// #define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your master's setup()


#include <Arduino.h>
#include "util/I2Cwrapper.h"


#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log



// Pin commands
const uint8_t pinPinModeCmd         = 70;
const uint8_t pinDigitalReadCmd     = 71; const uint8_t pinDigitalReadResult = 2; // 1 "int"
const uint8_t pinDigitalWriteCmd    = 72;
const uint8_t pinAnalogReadCmd      = 73; const uint8_t pinAnalogReadResult  = 2; // 1 "int"
const uint8_t pinAnalogWriteCmd     = 74;
const uint8_t pinAnalogReferenceCmd = 75;

/*!
  @brief An I2C wrapper class for remote analog and digital pin control.
  @details
  This class mimicks the original pin read and write commands.
  It replicates all of the methods and transmits each method call via I2C to
  a slave running the appropriate @ref firmware.ino firmware, given that pin control
  support is enabled at compile time.
  Functions and parameters without documentation will work just as their original,
  but you need to take the general restrictions into account (e.g. don't take a return
  value for valid without error handling).
  @note ESP32 needs ESP32 Arduino 2.0.1 (see board manager) or higher for native 
  analogWrite() support.
  @note analogReference() is only implemented for AVRs, ESPs don't have it. 
  @todo Only tested on AVRs.
  */
class PinI2C
{
public:
  
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the slave the pin is connected to.
   */
  PinI2C(I2Cwrapper* w);

  void pinMode(uint8_t, uint8_t);
  void digitalWrite(uint8_t, uint8_t);
  int digitalRead(uint8_t);
  int analogRead(uint8_t);
  void analogReference(uint8_t mode);
  void analogWrite(uint8_t, int);
  
private:
  I2Cwrapper* wrapper;

};


#endif
