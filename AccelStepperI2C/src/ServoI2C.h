/*!
  @file ServoI2C.h
  @brief Arduino library for I2C-control of servo motors connected to
  another Arduino which runs the associated @ref firmware.ino "AccelStepperI2C firmware".
  This is more or less a byproduct of the AccelStepperI2C project, but could be
  used as a (slightly bloated) standalone solution for I2C control of servo motors.
  Just as AccelStepperI2C, ServoI2C mimicks the interface of the original
  Arduino Servo libraries, so that it can be used with only little adaptations.
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
  @todo AccelStepperI2C and ServoI2C should be derived from a common base class,
  atm there's some unhealthy copy and paste
*/


#ifndef ServoI2C_h
#define ServoI2C_h

// #define DEBUG // uncomment for debug output to Serial (which has to be begun() in the main sketch)

#include <Arduino.h>
#include <I2Cwrapper.h>
#include <SimpleBuffer.h>

#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG

const uint8_t ServoI2CmaxBuf = 20; // upper limit of send and receive buffer(s)

// Servo commands
const uint8_t servoAttach1Cmd           = 60; const uint8_t servoAttachResult            = 1; // 1 uint8_t
const uint8_t servoAttach2Cmd           = 61; // uses servoAttachResult as well.
const uint8_t servoDetachCmd            = 62;
const uint8_t servoWriteCmd             = 63;
const uint8_t servoWriteMicrosecondsCmd = 64;
const uint8_t servoReadCmd              = 65; const uint8_t servoReadResult              = 2; // 1 int
const uint8_t servoReadMicrosecondsCmd  = 66; const uint8_t servoReadMicrosecondsResult  = 2; // 1 int
const uint8_t servoAttachedCmd          = 67; const uint8_t servoAttachedResult          = 1; // 1 bool


/*!
  @brief An I2C wrapper class for the Arduino [Servo
  library](https://www.arduino.cc/reference/en/libraries/servo/).
  @details
  This class mimicks the [original Servo interface](https://www.arduino.cc/reference/en/libraries/servo/).
  It replicates most of its methods and transmits each method call via I2C to
  a slave running the @ref firmware.ino "AccelStepperI2C firmware".
  Functions and parameters without documentation will work just as their original,
  but you need to take the general restrictions into account (e.g. don't take a return
  value for valid without error handling).

  Expect servo methods without dedicated documentation to work just like
  their non-I2C original.
  @note Currently, the implementation is quite dumb, as it passes any Servo library
  call over I2C, regardless of what it does. Write() e.g. uses WriteMicroseconds(),
  so that in theory it could be implemented wholly on the master's side.
  */
class ServoI2C
{
public:
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the slave the servo is connected to.
   */
  ServoI2C(I2Cwrapper* w);

  /*!
   * @brief Attach and prepare servo for use. Similar to the original, but see below.
   * @param pin Hardware pin of the *slave* the servo is atached to (see note).
   * @result Returns @ref myNum, which should be the same as the servo channel
   * returned by the original Servo libraries, except if you reset your master
   * without resetting the slave. Check for @ref myNum != 255 to see if the slave
   * successfully added the servo.
   * @note The slave's platform pin names like "D3" or "A1" might not be known
   * to the master's platform, if both platforms are different, or they might be
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
