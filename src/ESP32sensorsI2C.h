/*!
  @file ESP32sensorsI2C.h
  @brief Arduino library for I2C-control of touch buttons and hall sensors
  of an ESP32 which runs the associated @ref firmware.ino firmware.
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
*/


#ifndef ESP32sensorsI2C_h
#define ESP32sensorsI2C_h

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



// ESP32sensors commands (reserved 070 - 074 ESP32sensorsI2C)
const uint8_t ESP32sensorsCmdOffset           = 70;
const uint8_t ESP32sensorsTouchSetCyclesCmd   = ESP32sensorsCmdOffset + 0;
const uint8_t ESP32sensorsTouchReadCmd        = ESP32sensorsCmdOffset + 1; const uint8_t ESP32sensorsTouchReadResult = 2; // 1 uint16_t
const uint8_t ESP32sensorsEnableInterruptsCmd = ESP32sensorsCmdOffset + 2;
const uint8_t ESP32sensorsHallReadCmd         = ESP32sensorsCmdOffset + 3; const uint8_t ESP32sensorsHallReadResult  = 2; // 1 int16_t ("int")
const uint8_t ESP32sensorsTemperatureReadCmd  = ESP32sensorsCmdOffset + 4; const uint8_t ESP32sensorsTemperatureReadResult  = 4; // 1 float

/*!
 * @ingroup InterruptReasons
 * @{
 */
const uint8_t interruptReason_ESP32sensorsTouch = 4;
/*!
 * @}
 */

/*!
  @brief An I2C wrapper class for ESP32 sensors (touch, temp, hall).
  @details
  This class mimicks the original Arduino ESP32 core commands.
  It replicates all of the methods and transmits each method call via I2C to
  a target running the appropriate @ref firmware.ino firmware, given that ESP32
  sensors support is enabled at compile time.
  Functions and parameters without documentation will work just as their original,
  but you need to take the general restrictions into account (e.g. don't take a return
  value for valid without error handling).
  */
class ESP32sensorsI2C
{
public:
  
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the target the pin is connected to.
   */
  ESP32sensorsI2C(I2Cwrapper* w);

  void touchSetCycles(uint16_t measure, uint16_t sleep);
  
  /*!
   * @brief Like the original.
   * @param pin Note, again, that if your controller is not an ESP32, the pin names 
   * T0..T9 will not be known, so use their raw pin equivalents.
   */
  uint16_t touchRead(uint8_t pin);

  /*!
   * @brief Of course, you can't set your own callback funtion for the target.
   * Instead, you can make a pin trigger an interrupt using the wrapper's global 
   * interrupt pin. Make sure to have defined it before per wrapper.setInterruptPin().
   * Will use interruptReason_ESP32sensorsTouch as interrupt reason and *touch 
   * pin* number (i.e. 0 to 9, for T0 to T9, not the raw hardware pin number) as unit.
   * @param pin
   * @param threshold
   * @param falling [reserved, not usable yet with current ESP32 core]
   * true to trigger on signal falling below the threshold 
   * (= touched), false to trigger on signal rising above threshold (= released).
   * Uses touchInterruptSetThresholdDirection().
   */
  void enableInterrupts(uint8_t pin, uint16_t threshold, bool falling = true);
  int hallRead();
  float temperatureRead();
  
private:
  I2Cwrapper* wrapper;

};


#endif
