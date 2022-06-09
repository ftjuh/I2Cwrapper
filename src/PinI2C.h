/*!
  @file PinI2C.h
  @brief Arduino library for I2C-control of digital and analog pins connected to
  another Arduino which runs the I2Cwrapper @ref firmware.ino with the 
  PinI2C module enabled.
  
  Just as AccelStepperI2C, PinI2C mimicks the interface of the original
  Arduino pin control functions, so that it can be used with very few adaptations.
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
  @todo implement interrupts?
*/


#ifndef PinI2C_h
#define PinI2C_h

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



// Pin commands (reserved 060 - 069 PinI2C)
const uint8_t pinCmdOffset          = 60;
const uint8_t pinPinModeCmd         = pinCmdOffset + 0;
const uint8_t pinDigitalReadCmd     = pinCmdOffset + 1; const uint8_t pinDigitalReadResult = 2; // 1 "int"
const uint8_t pinDigitalWriteCmd    = pinCmdOffset + 2;
const uint8_t pinAnalogReadCmd      = pinCmdOffset + 3; const uint8_t pinAnalogReadResult  = 2; // 1 "int"
const uint8_t pinAnalogWriteCmd     = pinCmdOffset + 4;
const uint8_t pinAnalogReferenceCmd = pinCmdOffset + 5;

/*!
  @brief An I2C wrapper class for remote analog and digital pin control.
  @details
  This class mimicks the original pin read and write commands.
  It replicates all of the methods and transmits each method call via I2C to
  a target running the @ref firmware.ino firmware, given that the pin control
  module is enabled at compile time.
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
   * @param w Wrapper object representing the target the pin is connected to.
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
