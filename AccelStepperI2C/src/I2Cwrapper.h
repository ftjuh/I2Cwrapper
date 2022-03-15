/*!
 *  @file I2Cwrapper.h
 *  @brief A helper class for the AccelStepperI2C (and ServoI2C) library.
 *  @section author Author
 *  Copyright (c) 2022 juh
 *  @section license License
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2.
 *  @todo Enhance diagnostics with a self-diagnosing function to determine the
 *  optimal/minimal I2Cdelay in a given master-slave setup.
 *  @todo <del>AccelStepperI2C and ServoI2C should be derived from a common base class,
 *  atm there's some unhealthy copy and paste</del>implemented with I2Cwrapper, but
 *  with client instead of inheritance relation.
 */

#ifndef I2Cwrapper_h
#define I2Cwrapper_h

//#define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your setup()


#include <SimpleBuffer.h>
#include "version.h"

#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log


const uint16_t maxBufDefault = 20; // includes 1 byte for CRC8

// ms to wait between I2C communication, can be changed by setI2Cdelay()
const unsigned long I2CdefaultDelay = 10;

// I2C commands used by the wrapper
const uint8_t resetCmd              = 241;
const uint8_t changeI2CaddressCmd   = 242;
const uint8_t setInterruptPinCmd    = 243;
const uint8_t clearInterruptCmd     = 244; const uint8_t clearInterruptResult    = 1; // 1 uint8_t
const uint8_t getVersionCmd         = 245; const uint8_t getVersionResult        = 4; // 1 uint32_t

/*!
 * @defgroup InterruptReasons  List of possible reasons an interrupt was triggered.
 * @brief Used by clearInterrup() to inform the master about what caused the
 * previous interrupt.
 */

/*!
 * @ingroup InterruptReasons
 */
const uint8_t interruptReason_none = 0; ///< You should not encounter this, as you don't want to be interrupted without a reason...

/*****************************************************************************/
/*****************************************************************************/

/*!
 * @brief A helper class for the AccelStepperI2C (and ServoI2C) library.
 *
 * I split
 * it from an earlier version of the AccelStepperI2C library when adding Servo
 * support, to be able to have a clean separation librarywise between the two:
 * The wrapper represents the I2C slave and handles all communication with it.
 * AccelStepperI2C and ServoI2C use it for communicating with the slave. To do
 * so, an I2Cwrapper object is instantiated with the slave's I2C address and
 * then passed to its clients',i.e. AccelStepperI2C or ServoI2C objects, constructor.
 * @par Command codes
 * * 000 - 009 (reserved)
 * * 010 - 059 AccelStepperI2C
 * * 060 - 069 ServoI2C
 * * 070 - 079 PinI2C
 * * 080 - 239 (unused)
 * * 240 - 255 I2Cwrapper commands (reset slave, change address etc.)
 * @par
 * Theoretically, new classes could use I2Cwrapper to add even more capabilities
 * to the slave, e.g. for driving DC motors, granted that the firmware is extended
 * accordingly and the above list of uint8_t command codes is kept free from duplicates.
 */
class I2Cwrapper
{
public:

  /*!
   * @brief Constructor.
   * @param i2c_address Address of the slave device
   * @param maxBuf Upper limit of send and receive buffer including 1 crc8 byte   *
   */
  I2Cwrapper(uint8_t i2c_address, uint8_t maxBuf = maxBufDefault);

  /*!
   * @brief Test if slave is listening.
   * @returns true if slave could be found under the given address.
   */
  bool ping();

  /*!
   * @brief Tells the slave to reset to it's default state. It is recommended to
   * reset the slave every time the master is started or restarted to put it in
   * a defined state. Else it could happen that the slave still manages units
   * (steppers/servos) which the master does not know about.
   */
  void reset();

  /*!
   * @brief Permanently change the I2C address of the device. New address is
   * stored in EEPROM (AVR) or flash memory (ESPs) and will be active after the
   * next reset/reboot.
   */
  void changeI2Caddress(uint8_t newAddress);

  /*!
   * @brief Define a global interrupt pin which can be used by device units
   * (steppers, servos...) to inform the master that something important happend.
   * Currently used by AccelStepperI2C to inform about end stop hits and target
   * reached events.
   * @param pin Pin the slave will use to send out interrupts.
   * @param activeHigh If true, HIGH will signal an interrupt.
   * @sa AccelStepperI2C::enableInterrupts()
   */
  void setInterruptPin(int8_t pin, bool activeHigh = true);

  /*!
   * @brief Acknowledge a received interrupt to slave so that it can clear the
   * interupt condition and return the reason for the interrupt.
   * @returns Reason for the interrupt as 8bit BCD with triggering unit in lower
   * 4 bits and trigger reason in the upper 4 bits. 0xff in case of error.
   * @sa InterruptReasons
   */
  uint8_t clearInterrupt();


  /*!
   * @brief Define a minimum time that the master waits between I2C transmissions.
   * This is to make sure that the slave has finished its earlier task or has
   * its answer to the master's previous command ready. Particularly for ESP32
   * slaves this is critical, as due to its implementation of I2C slave mode,
   * an ESP32 could theoretically send incomplete data if a request is sent too
   * early. The actual delay will take the time spent since the last I2C 
   * transmission into account, so that it won't wait at all if the given time 
   * has already passed.
   * @param delay Minimum time in between I2C transmissions in milliseconds.
   * The default I2CdefaultDelay is a bit conservative at 10 ms to allow for 
   * for serial debugging output to slow things down. You can try to go lower, 
   * if slave debugging is disabled.
   * @return Returns the previously set delay.
   * @todo <del>I2Cdelay is currently global; make it a per-slave setting.</del>
   * implemented with I2Cwrapper.
   */
  unsigned long setI2Cdelay(unsigned long delay);

  /*!
   * @brief Get semver compliant version of slave firmware.
   * @returns major version in bits 0-7, minor version in bits 8-15; patch version in bits 16-23;  0xFFFFFFFF on error.
   */
  uint32_t getSlaveVersion();


  /*!
   * @brief Get version of slave firmware and compare it with library version.
   * @returns true if both versions match, i.a. are compatible.
   */
  bool checkVersion(uint32_t masterVersion);

  /*!
     * @brief Return and reset the number of failed commands sent since the last
     * time this method was used. A command is sent each time a function call
     * is transmitted to the slave.
     * @sa resultErrors(), transmissionErrors()
     */
  uint16_t sentErrors();

  /*!
   * @brief Return and reset the number of failed receive events since the last
   * time this method was used. A receive event happens each time a function
   * returns a value from the slave.
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
  bool sentOK = false;   ///< True if previous function call was successfully transferred to slave.
  bool resultOK = false; ///< True if return value from previous function call was received successfully

private:
  void doDelay();
  uint8_t address;
  // ms to wait between I2C communication, can be changed by setI2Cdelay()
  unsigned long I2Cdelay = I2CdefaultDelay;
  unsigned long lastI2Ctransmission = 0; // used to adjust I2Cdelay in doDelay()
  uint16_t sentErrorsCount = 0;   // Number of transmission errors. Will be reset to 0 by sentErrors().
  uint16_t resultErrorsCount = 0; // Number of receiving errors. Will be reset to 0 by resultErrors().
};



#endif
