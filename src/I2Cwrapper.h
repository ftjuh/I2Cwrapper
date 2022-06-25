/*!
 * @file I2Cwrapper.h
 * @brief Core helper class of the I2Cwrapper framework. Handles target 
 * device management and I2C communication on the controller's side
 * @section author Author
 * Copyright (c) 2022 juh
 * @section license License
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 * @todo Enhance diagnostics with a self-diagnosing function to determine the
 * optimal/minimal I2Cdelay in a given controller-target setup.
 */

#ifndef I2Cwrapper_h
#define I2Cwrapper_h

// #define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your controller's setup()


#include "util/SimpleBuffer.h"
#include "util/version.h"

#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log


const uint8_t I2CmaxBuf = 20; // upper limit of send and receive buffer(s), includes 1 byte for CRC8


// ms to wait between I2C communication, can be changed by setI2Cdelay()
const unsigned long I2CdefaultDelay = 20; // must be <256

// number of repetitions used in autoAdjustI2Cdelay()
const uint8_t autoAdjustDefaultReps = 10;

// I2C commands used by the wrapper
const uint8_t resetCmd              = 241;
const uint8_t changeI2CaddressCmd   = 242;
const uint8_t setInterruptPinCmd    = 243;
const uint8_t clearInterruptCmd     = 244; const uint8_t clearInterruptResult    = 1; // 1 uint8_t
const uint8_t getVersionCmd         = 245; const uint8_t getVersionResult        = 4; // 1 uint32_t
const uint8_t pingBackCmd           = 246; // has variable result length, so no const uint8_t pingBackResult

/*!
 * @defgroup InterruptReasons  List of possible reasons an interrupt was triggered.
 * @brief Used by clearInterrup() to inform the controller about what caused the
 * previous interrupt.
 */

/*!
 * @ingroup InterruptReasons
 */
const uint8_t interruptReason_none = 0; ///< You should not encounter this in practice, as you don't want to be interrupted without a reason...

/*****************************************************************************/
/*****************************************************************************/

/*!
 * @brief A helper class for the AccelStepperI2C and related libraries.
 *
 * I split
 * it from an earlier version of the AccelStepperI2C library when adding Servo
 * support, to be able to have a clean separation librarywise between the two:
 * The wrapper represents the I2C target and handles all communication with it.
 * AccelStepperI2C, ServoI2C, and others use it for communicating with the target. To do
 * so, an I2Cwrapper object is instantiated with the target's I2C address and
 * then passed to its client object's (AccelStepperI2C etc.) constructor.
 * @par Command codes
 * * 000 - 009 (reserved)
 * * 010 - 049 AccelStepperI2C
 * * 050 - 059 ServoI2C
 * * 060 - 069 PinI2C
 * * 070 - 074 ESP32sensorsI2C
 * * 075 - 079 (unused)
 * * 080 - 089 TM1638liteI2C
 * * 090 - 239 (unused)
 * * 240 - 255 I2Cwrapper commands (reset target, change address etc.)
 * @par
 * New classes could use I2Cwrapper to easily add even more capabilities
 * to the target, e.g. for driving DC motors, granted that the firmware is extended
 * accordingly and the above list of uint8_t command codes is kept free from duplicates.
 */
class I2Cwrapper
{
public:

  /*!
   * @brief Constructor.
   * @param i2c_address Address of the target device
   * @param maxBuf Upper limit of send and receive buffer including 1 crc8 byte
   */
  I2Cwrapper(uint8_t i2c_address, uint8_t maxBuf = I2CmaxBuf);

  /*!
   * @brief Test if target device is listening.
   * @returns true if target could be found under the given address.
   */
  bool ping();

  /*!
   * @brief Tells the target device to reset to its default state. It is 
   * recommended to reset the target every time the controller is started or restarted 
   * to put it in a defined state. Else it could happen that the target still 
   * manages units (steppers, etc.) which the controller does not know about.
   */
  void reset();

  /*!
   * @brief Permanently change the I2C address of the device. The new address is
   * stored in EEPROM (AVR) or flash memory (ESPs) and will be active after the
   * next reset/reboot.
   */
  void changeI2Caddress(uint8_t newAddress);

  /*!
   * @brief Define a global interrupt pin which can be used by device units
   * (steppers, servos...) to inform the controller that something important happend.
   * Currently used by AccelStepperI2C to inform about end stop hits and target
   * reached events, and ESP32sensorsI2C to inform a touched button event.
   * @param pin Pin the target will use to send out interrupts.
   * @param activeHigh If true, HIGH will signal an interrupt.
   * @sa AccelStepperI2C::enableInterrupts()
   */
  void setInterruptPin(int8_t pin, bool activeHigh = true);

  /*!
   * @brief Acknowledge to target that interrupt has been received, so that the 
   * target can clear the interupt condition and return the reason for the 
   * interrupt.
   * @returns Reason for the interrupt as 8bit BCD with triggering unit in lower
   * 4 bits and trigger reason in the upper 4 bits. 0xff in case of error.
   * @sa InterruptReasons
   */
  uint8_t clearInterrupt();

  /*!
   * @brief Define a minimum duration of time that the controller keeps between 
   * I2C transmissions.
   * This is to make sure that the target has finished its earlier task or has
   * its answer to the controller's previous command ready. Particularly for ESP32
   * targets this is critical, as due to its implementation of I2C target mode,
   * an ESP32 could theoretically send incomplete data if a request is sent too
   * early. The actual delay will take the time spent since the last I2C 
   * transmission into account, so that it won't wait at all if the given time 
   * has already passed.
   * @param delay Minimum time in between I2C transmissions in milliseconds.
   * The default I2CdefaultDelay is a bit conservative at 10 ms to allow for 
   * for serial debugging output to slow things down. You can try to go lower, 
   * if target debugging is disabled.
   * @return Returns the previously set delay.
   * @see autoAdjustI2Cdelay()
   */
  unsigned long setI2Cdelay(unsigned long delay);
  
  /*!
   * @brief Returns currently set I2Cdelay.
   */
  unsigned long getI2Cdelay();
  
  /*!
   * @brief Set I2C delay to the smallest value that still allows error
   * free transmissions. To determine it, a simulation test is used: A number
   * of dummy transmissions are repeatedly exchanged with the target using the
   * pingBack() function, while successively increasing the I2C delay, starting 
   * at 1 ms. The test stops if all repetitions came back error free with the 
   * currently used I2C delay, or if the maxI2Cdelay constant is reached. This 
   * value, increased by the safety margin, or maxI2Cdelay, will be set as the 
   * new I2C delay.
   * @param safetyMargin A number of microseconds that will be added to the 
   * empirically determined minimum I2C delay. As the test transmissions do 
   * nothing but send back the amount of specified simulated parameter bytes, 
   * you will want to specify some extra time to allow for the time the controller
   * will need to process any given command on top of the pure transmissions.
   * Its optimal value fully depends on how fast the target's module(s) do their
   * job. It should usually be at least 1 ms, for the included modules 2 ms are
   * a safe bet, that's why it's used as the default. 
   * @param maxLength Number of simulated test parameter bytes sent with each
   * testing transmission, defaulting to the maximum bytes that are possible 
   * with the given I2C buffer size. This theoretical maximum (I2CmaxBuf minus 
   * three bytes for the message header) will not be fully used by most modules. 
   * So if you know what the maximum number of parameter bytes sent or reveiced 
   * by any of the commands you will use in your project is, you can 
   * specify it here to get a more aggressive, shorter I2C delay. Leave it to 
   * the default to be on the safe side, it should not make a diference of more
   * than 1 ms.
   * @note new in v0.3.0, experimental
   * @return The newly set I2C delay
   * @see setI2Cdelay()
   */
  uint8_t autoAdjustI2Cdelay(uint8_t safetyMargin = 2, uint8_t maxLength = I2CmaxBuf - 3);  

  /*!
   * @brief Get semver compliant version of target firmware.
   * @returns major version in bits 0-7, minor version in bits 8-15; patch version in bits 16-23;  0xFFFFFFFF on error.
   */
  uint32_t getVersion();

  /*!
   * @brief Get version of target firmware and compare it with library version.
   * @returns true if both versions match, i.a. are compatible.
   */
  bool checkVersion(uint32_t controllerVersion);

  /*!
     * @brief Return and reset the number of failed commands sent since the last
     * time this method was used. A command is sent each time a function call
     * is transmitted to the target.
     * @sa resultErrors(), transmissionErrors()
     */
  uint16_t sentErrors();

  /*!
   * @brief Return and reset the number of failed receive events since the last
   * time this method was used. A receive event happens each time a function
   * returns a value from the target.
   * @sa sentErrors(), transmissionErrors()
   */
  uint16_t resultErrors();

  /*!
   * @brief Return and reset the sum of failed commands sent *and* failed receive
   * events since the last time this method was used. Use this if you are only
   * interested in the sum of all transmission errors, not in what direction
   * the errors occurred.
   * @result Sum of sentErrors() and resultErrors()
   * @sa sentErrors(), resultErrors()
   */
  uint16_t transmissionErrors();

  void prepareCommand(uint8_t cmd, uint8_t unit = -1);
  bool sendCommand();
  bool readResult(uint8_t numBytes);

  SimpleBuffer buf;
  bool sentOK = false;   ///< True if previous function call was successfully transferred to target.
  bool resultOK = false; ///< True if return value from previous function call was received successfully

private:
  
  /*!
   * @brief Diagnostic function to determine optimal I2C delay, meant for
   * internal use by the autoAdjustI2Cdelay() function. Makes the target
   * send back a given data byte a given number of times.
   * @param testData ony byte data to send back to the controller
   * @param testLength number of times the test byte is sent back. This is to
   * simulate commands with arbitrary amounts of returned data. Internally, the
   * test byte will be incremented by 73 with each position, however this is 
   * transparent to the user and is meant to more closely simulate real world 
   * data than simple repetitions.
   * @return true if data was received correctly back from the target
   */
  bool pingBack(uint8_t testData, uint8_t testLength);
  
  void doDelay();
  uint8_t address;
  // ms to wait between I2C communication, can be changed by setI2Cdelay()
  unsigned long I2Cdelay = I2CdefaultDelay;
  unsigned long lastI2Ctransmission = 0; // used to adjust I2Cdelay in doDelay()
  uint16_t sentErrorsCount = 0;   // Number of transmission errors. Will be reset to 0 by sentErrors().
  uint16_t resultErrorsCount = 0; // Number of receiving errors. Will be reset to 0 by resultErrors().
};



#endif
