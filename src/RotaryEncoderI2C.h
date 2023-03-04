/*!
  @file RotaryEncoderI2C.h
  @brief I2Cwrapper module which allows I2C control/use of up to 8 quadrature 
  encoders over I2C. Uses the RotaryEncoder library (https://github.com/mathertel/RotaryEncoder) 
  by Matthias Hertel (http://www.mathertel.de).
  
  ## Author
  Copyright (c) 2023 juh
  ## License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#ifndef RotaryEncoderI2C_h
#define RotaryEncoderI2C_h

// #define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your controller's setup()


#include <Arduino.h>
#include <I2Cwrapper.h>
#include <RotaryEncoder.h>

#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log


// RotaryEncoderI2C commands
const uint8_t rotaryEncoderCmdOffset = 130;
const uint8_t rotaryEncoderAttachCmd  = rotaryEncoderCmdOffset + 0;  const uint8_t rotaryEncoderAttachCmdResult = 1; // uint8_t (myNum)
const uint8_t rotaryEncoderGetPositionCmd  = rotaryEncoderCmdOffset + 1; const uint8_t rotaryEncoderGetPositionCmdResult = 4; // long
const uint8_t rotaryEncoderGetDirectionCmd  = rotaryEncoderCmdOffset + 2; const uint8_t rotaryEncoderGetDirectionCmdResult = 1; // Direction enum, we'll convert it to 1 int8_t
const uint8_t rotaryEncoderSetPositionCmd  = rotaryEncoderCmdOffset + 3;
const uint8_t rotaryEncoderGetMillisBetweenRotationsCmd  = rotaryEncoderCmdOffset + 4; const uint8_t rotaryEncoderGetMillisBetweenRotationsCmdResult = 4; // unsigned long
const uint8_t rotaryEncoderGetRPMCmd  = rotaryEncoderCmdOffset + 5; const uint8_t rotaryEncoderGetRPMCmdResult =  4; // unsigned long
const uint8_t rotaryEncoderStartDiagnosticsModeCmd  = rotaryEncoderCmdOffset + 6;
//const uint8_t rotaryEncoderCmd  = rotaryEncoderCmdOffset + 7;
//const uint8_t rotaryEncoderCmd  = rotaryEncoderCmdOffset + 8;
//const uint8_t rotaryEncoderCmd  = rotaryEncoderCmdOffset + 9;


/*!
  @brief An I2C wrapper class for quadrature rotary sensors which uses the 
  RotaryEncoder library by Matthias Hertel
  @details Each rotary sensor is represented by an I2Cwrapper unit. Supports up 
  to 2 sensors on ATtiny85, 8 sensors otherwise.
  */
class RotaryEncoderI2C
{
public:
  
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the target the pin is connected to.
   */
  RotaryEncoderI2C(I2Cwrapper* w);

  /*!
   * @brief Replaces the RotaryEncoder constructor and takes the same arguments.
   * Will allocate an RotaryEncoder object on the target's side and make it ready for use.
   * Does not return an success/error result. Instead check @ref myNum >= 0 to 
   * see if the target successfully added the encoder. If not, it's -1.
   * @param pin1,pin2,pin3,mode see original library
   * @returns total number of attached encoders including this one, -1 in case
   * of error (= limit of attachable encoders exceeded)
   */
  int8_t attach(int pin1, int pin2, RotaryEncoder::LatchMode mode = RotaryEncoder::LatchMode::FOUR0);
  
  long getPosition();

  /*!
   * @brief Important: In normal RotaryEncounter, direction will be
   * the direction during at the moment of the last tick(). It will
   * thus correspond with the getPosition() reading. In I2C mode,
   * the target will tick() on its own. So the direction will be
   * more current than in normal mode and may seem to contradict
   * the position reading. Needs fixing.
   * @returns     NOROTATION = 0, CLOCKWISE = 1, COUNTERCLOCKWISE = -1
   */
  RotaryEncoder::Direction getDirection();
  
  void setPosition(long newPosition);
  unsigned long getMillisBetweenRotations() const;
  
  /*!
   * @brief Note that I think getRPM() is not functional in the original library, as it assumes a fixed
   * number of 20 pulses per rotation. See [this issue](https://github.com/mathertel/RotaryEncoder/issues/40)
   * @returns (probably wrong) RPM
   */
  unsigned long getRPM();
  
  /*!
   * @brief Puts device in a diagnostics mode which is meant for analysing your quadrature
   * signal. After calling this, the device will not accept any commands anymore (so don't send 
   * any after that), it will just serve onRequest() events and reply to them by sending one 
   * single byte which contains the current pin state of the selected encoders' two input pins. 
   * Use getDiagnostics() for that. You'll need to reset the device to put it back in normal mode.
   * @param numEncoder number of the encoder to diagnose, default is the first one
   * @sa getDiagnostics()
   * @note ESP32's weird I2C target implementation could break this, so be warned that diagnostics
   * mode might not work on this platform.
   */
  void startDiagnosticsMode(uint8_t numEncoder = 0);
  
  /*!
   * @brief Use only in diagnostics mode to read the status of the two input pins of your chosen 
   * attached encoder.
   * @returns (pin1 | (pin2 << 1)) of the encoder you chose in startDiagnosticsMode(); 0xFF in case 
   * of error
   * @sa startDiagnosticsMode()
   */
  uint8_t getDiagnostics();
    
  int8_t myNum = -1;    // for modules that support units, i.e. more than one instance of the hardware represented by this class
  
private:
  I2Cwrapper* wrapper;

};


#endif
