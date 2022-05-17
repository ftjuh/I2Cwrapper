/*!
  @file ServoI2C.h
  @brief Arduino library for I2C-control of servo motors connected to
  another Arduino which runs the I2Cwrapper @ref firmware.ino with the 
  ServoI2C module enabled.
  
  Just as AccelStepperI2C, ServoI2C mimicks the interface of the original
  Arduino Servo libraries, so that it can be used with only minor adaptations
  to non-I2C code.
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#ifndef ServoI2C_h
#define ServoI2C_h

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



// Servo commands (reserved 050 - 059 ServoI2C)
const uint8_t servoCmdOffset            = 50;
const uint8_t servoAttach1Cmd           = servoCmdOffset + 0; const uint8_t servoAttachResult            = 1; // 1 uint8_t
const uint8_t servoAttach2Cmd           = servoCmdOffset + 1; // uses servoAttachResult, too.
const uint8_t servoDetachCmd            = servoCmdOffset + 2;
const uint8_t servoWriteCmd             = servoCmdOffset + 3;
const uint8_t servoWriteMicrosecondsCmd = servoCmdOffset + 4;
const uint8_t servoReadCmd              = servoCmdOffset + 5; const uint8_t servoReadResult              = 2; // 1 int
const uint8_t servoReadMicrosecondsCmd  = servoCmdOffset + 6; const uint8_t servoReadMicrosecondsResult  = 2; // 1 int
const uint8_t servoAttachedCmd          = servoCmdOffset + 7; const uint8_t servoAttachedResult          = 1; // 1 bool


/*!
  @brief An I2C wrapper class for the Arduino [Servo
  library](https://www.arduino.cc/reference/en/libraries/servo/).
  @details
  This class mimicks the [original Servo interface](https://www.arduino.cc/reference/en/libraries/servo/).
  It replicates most of its methods and transmits each method call via I2C to
  a target running the I2Cwrapper @ref firmware.ino with the ServoI2C module enabled.
  Functions and parameters without documentation will work just as their original,
  but you need to take the general restrictions into account (e.g. don't take a return
  value for valid without error handling).
  @note Currently, the implementation is quite dumb, as it passes any Servo library
  call over I2C, regardless of what it does. Write() e.g. uses WriteMicroseconds(),
  so that in theory it could be implemented wholly on the controller's side.
  */
class ServoI2C
{
public:
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the target the servo is connected to.
   */
  ServoI2C(I2Cwrapper* w);

  /*!
   * @brief Attach and prepare servo for use. Similar to the original, but see below.
   * @param pin Hardware pin of the *target* the servo is atached to (see note).
   * @result Returns @ref myNum, which should be the same as the servo channel
   * returned by the original Servo libraries, except if you reset your controller
   * without resetting the target. Check for @ref myNum != 255 to see if the target
   * successfully added the servo.
   * @note The target's platform pin names like "D3" or "A1" might not be known
   * to the controller's platform, if both platforms are different, or they might be
   * used with a different pin number. So it is safer to use integer equivalents
   * as defined in the respective platform's `pins_arduino.h`, see
   * AccelStepperI2C::attach()
   */
  uint8_t attach(int pin);
  uint8_t attach(int pin, int min, int max);
  void detach();
  void write(int value);
  void writeMicroseconds(int value);
  int read();
  int readMicroseconds();
  bool attached();

  int8_t myNum = -1;    ///< Servo number with myNum >= 0 for successfully added servo.

private:
  I2Cwrapper* wrapper;

};


#endif
