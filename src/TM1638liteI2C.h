/*!
  @file TM1638liteI2C.h
  @brief Arduino library for I2C-control of one or more TM1638 LED/key modules
  connected to another Arduino which runs the I2Cwrapper @ref firmware.ino with 
  the TM1638liteI2C module enabled. TM1638liteI2C mimicks the interface of Danny 
  Ayers' TM1638lite library, so that it can be used with only minor adaptations 
  to non-I2C code.  
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#ifndef TM1638liteI2C_h
#define TM1638liteI2C_h

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


// TM1638liteI2C commands
const uint8_t TM1638liteCmdOffset       = 75; 
const uint8_t TM1638liteAttachCmd       = TM1638liteCmdOffset + 0; const uint8_t TM1638liteAttachResult = 1; // 1 uint8_t
const uint8_t TM1638liteSendCommandCmd  = TM1638liteCmdOffset + 1;
const uint8_t TM1638liteResetCmd        = TM1638liteCmdOffset + 2;
const uint8_t TM1638liteReadButtonsCmd  = TM1638liteCmdOffset + 3; const uint8_t TM1638liteReadButtonsResult = 1; // 1 uint8_t
const uint8_t TM1638liteSetLEDCmd       = TM1638liteCmdOffset + 4;
//const uint8_t TM1638liteDisplayTextCmd  = TM1638liteCmdOffset + ; // implemented locally at controller's side; calls TM1638liteDisplayASCII()
const uint8_t TM1638liteDisplaySSCmd    = TM1638liteCmdOffset + 5;
const uint8_t TM1638liteDisplayASCIICmd = TM1638liteCmdOffset + 6;
const uint8_t TM1638liteDisplayHexCmd   = TM1638liteCmdOffset + 7;


/*!
  @brief An I2C wrapper class for Danny Ayers' TM1638lite library 
  (https://www.arduino.cc/reference/en/libraries/tm1638lite/)
  */
class TM1638liteI2C
{
public:
  
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the target a TM1638 is connected to.
   */
  TM1638liteI2C(I2Cwrapper* w);

  /*!
   * @brief Replaces the TM1638lite constructor and takes the same arguments.
   * Will allocate an TM1638lite object on the target's side and make it ready for use.
   * @param strobe, clock, data See original library
   * @result Check @ref myNum >= 0 to see if the target successfully added the
   * object. If not, it's -1.
   * @note The target's platform pin names might not be known to the controller's 
   * platform, if both are different. So it is safer to use integer equivalents
   * as defined in the respective platform's `pins_arduino.h`
   */
  void attach(uint8_t strobe, uint8_t clock, uint8_t data);
  void sendCommand(uint8_t value);
  void reset();
  uint8_t readButtons();
  void setLED(uint8_t position, uint8_t value);
  /*!
   * @note: displayText() is implemented locally at the controller's side
   * to avoid having to send arbitrary long strings. This happens transparently 
   * for the caller and might only lead to problems if the original library's 
   * code changes.
   */
  void displayText(String text);
  void displaySS(uint8_t position, uint8_t value);
  void displayASCII(uint8_t position, uint8_t ascii);
  void displayHex(uint8_t position, uint8_t hex);
  
  int8_t myNum = -1;

private:
  I2Cwrapper* wrapper;
};


#endif
