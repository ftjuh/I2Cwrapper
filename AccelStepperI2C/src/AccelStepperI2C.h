/*!
  @file AccelStepperI2C.h
  @brief Arduino library for I2C-control of stepper motors connected to
  another Arduino which runs the associated AccelStepperI2C firmware.
  @par Functions with an empty documentation will work just as their original, but 
  you need to take the general restrictions into account (e.g. don't take a return 
  value for valid unless checking for resultOK)
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
  @todo use interrupts for endstops instead of main loop polling
  @todo Versioning, I2C command to request version (important, as library and firmware
  always need to match)
  @todo add emergency stop/break pin for slave
  @todo make time the slave has to answer I2C requests (I2CrequestDelay)
  configurable, as it will depend on µC and bus frequency etc.
  @todo checking each transmission with sentOK and resultOK is tedious. We could
  use some counter to accumulate errors and check them summarily, e.g. at
  the end of setup etc.
  @todo test (and adapt) slave firmware for ESP8266
  @todo ATM data is not protected against updates from ISRs while it is being
  used in the main program (see http://gammon.com.au/interrupts). Check if this
  could be a problem in our case.
  @todo clean up example sketches
  @todo ESP32: make use of dual cores?
  @todo <done>Implement interrupt mechanism, so that the slave can inform the master
  about finished tasks or other events</done> - done
  @todo <del>implement diagnostic functions, e.g. measurements how long messages take 
  to be processed or current stepper pulse frequency</del> - done, performance graphs
  included in documentation
  @todo <del>add ESP32 compatibility for slave</del> - done, but needs further testing
  @todo <del>Make the I2C address programmable and persistent in EEPROM.</del> - done
  @todo <del>Error handling and sanity checks are rudimentary. Currently the
  system is not stable against transmission errors, partly due to the limitations
  of Wire.requestFrom() and Wire.available(). A better protocol would be
  needed, which would mean more overhead. Need testing to see how important
  this is in practics.</del> - CRC8 implemented
  @todo <del>Implement end stops/reference stops.</del> - endstop polling 
  implemented, interrupt would be better
  @par Revision History
  @version 0.1 initial release
*/

#ifndef AccelStepperI2C_h
#define AccelStepperI2C_h

#include <Arduino.h>
#include <AccelStepper.h>
#include <SimpleBuffer.h>


// upper limit of send and receive buffer(s)
const uint8_t maxBuf = 20; // includes 1 crc8 byte

// ms to wait between I2C communication, this needs to be improved, a fixed constant is much too arbitrary
const uint16_t I2CrequestDelay = 50;

// used as return valuefor calls with long/float result that got no correct reply from slave
// However, errors are now signaled with resultOK, so no need for a special value here, just take 0
const long resError = 0;

/*!
 * @brief Used to transmit diagnostic info with AccelStepperI2C::diagnostics(). 
 */
struct diagnosticsReport {
  uint32_t cycles;          ///< Number of main loop executions since last reboot
  uint32_t lastProcessTime; ///< microseconds needed to process (interpret) most recently sent command
  uint32_t lastRequestTime; ///< microseconds spent in most recent onRequest() interrupt
  uint32_t lastReceiveTime; ///< microseconds spent in most recent onReceive() interrupt
};

// I2C commands and, if non void, returned bytes, for AccelStepper functions, starting at 10
// note: not all of those are necessarily implemented yet
const uint8_t moveToCmd             = 10;
const uint8_t moveCmd               = 11;
const uint8_t runCmd                = 12; const uint8_t runResult                 = 1; // 1 boolean
const uint8_t runSpeedCmd           = 13; const uint8_t runSpeedResult            = 1; // 1 boolean
const uint8_t setMaxSpeedCmd        = 14;
const uint8_t maxSpeedCmd           = 15; const uint8_t maxSpeedResult            = 4; // 1 float
const uint8_t setAccelerationCmd    = 16;
const uint8_t setSpeedCmd           = 17;
const uint8_t speedCmd              = 18; const uint8_t speedResult               = 4; // 1 float
const uint8_t distanceToGoCmd       = 19; const uint8_t distanceToGoResult        = 4; // 1 long
const uint8_t targetPositionCmd     = 20; const uint8_t targetPositionResult      = 4; // 1 long
const uint8_t currentPositionCmd    = 21; const uint8_t currentPositionResult     = 4; // 1 long
const uint8_t setCurrentPositionCmd = 22;
const uint8_t runToPositionCmd      = 23; // blocking
const uint8_t runSpeedToPositionCmd = 24;  const uint8_t runSpeedToPositionResult = 1; // 1 boolean
const uint8_t runToNewPositionCmd   = 25; // blocking
const uint8_t stopCmd               = 26;
const uint8_t disableOutputsCmd     = 27;
const uint8_t enableOutputsCmd      = 28;
const uint8_t setMinPulseWidthCmd   = 29;
const uint8_t setEnablePinCmd       = 30;
const uint8_t setPinsInverted1Cmd   = 31;
const uint8_t setPinsInverted2Cmd   = 32;
const uint8_t isRunningCmd          = 33; const uint8_t isRunningResult           = 1; // 1 boolean


// new commands for AccelStepperI2C start here

// commands for specific steppers, starting at 100
const uint8_t setStateCmd           = 100;
const uint8_t getStateCmd           = 101; const uint8_t getStateResult           = 1; // 1 uint8_t
const uint8_t setEndstopPinCmd      = 102;
const uint8_t enableEndstopsCmd     = 103;
const uint8_t endstopsCmd           = 104; const uint8_t endstopsResult           = 1; // 1 uint8_t

const uint8_t generalCommandsStart = 200; //not a command, just to define the 200 threshold needed to check if a valid stepper address is to be expected
// general commands that don't address a specific stepper (= don't use second header byte), starting at 200
const uint8_t addStepperCmd         = 200; const uint8_t addStepperResult         = 1; // 1 uint8_t
const uint8_t resetCmd              = 201;
const uint8_t changeI2CaddressCmd   = 202;
const uint8_t enableDiagnosticsCmd  = 203;
const uint8_t diagnosticsCmd        = 204; const uint8_t diagnosticsResult        = sizeof(diagnosticsReport);
const uint8_t setInterruptPinCmd    = 205;
const uint8_t enableInterruptsCmd   = 206;

/// @brief stepper state machine states
const uint8_t state_stopped             = 0; ///< state machine is inactive, stepper can still be controlled directly
const uint8_t state_run                 = 1; ///< corresponds to AccelStepper::run(), will fall back to state_stopped if target reached
const uint8_t state_runSpeed            = 2; ///< corresponds to AccelStepper::runSpeed(), will remain active until stopped by user or endstop
const uint8_t state_runSpeedToPosition  = 3; ///< corresponds to AccelStepper::state_runSpeedToPosition(), will fall back to state_stopped if target position reached



/*!
 * @brief Tells the slave to reset. Call this *before* adding any steppers *if*
 * master and slave resets are not synchronized by hardware. Else the slave
 * survives the master's reset, will think the master wants another stepper,
 * not a first one, and will run out of steppers, sooner or later.
 * @par All steppers are stopped by AccelStepper::stop() and disabled by
 * AccelStepper::disableOutputs(), after that a hardware reset is triggered.
 * @todo reset should be done entirely in software.
*/
void resetAccelStepperSlave(uint8_t address);

/*!
 * @brief Permanently change the I2C address of the device. New address is
 * stored in EEPROM and will be active after the next reset/reboot.
 */
void changeI2Caddress(uint8_t address, uint8_t newAddress);



class AccelStepperI2C {
 public:
  /*!
   * @brief Constructor. Can be used like the original , but needs an additional i2c address
   * parameter. Will allocate an AccelStepper ocject on the slave's side and make it ready for use.
   * @param i2c_address Address of the controller the stepper is connected to. The library should support multiple controllers with different addresses, this is still untested, though.
   * @param interface Only AccelStepper::DRIVER is tested at the moment, but AccelStepper::FULL2WIRE, AccelStepper::FULL3WIRE, AccelStepper::FULL4WIRE, AccelStepper::HALF3WIRE, and AccelStepper::HALF4WIRE should work as well, AccelStepper::FUNCTION of course not
   * @result For the constructor, instead of checking sentOK and resultOK for
   * success, you can just check if @ref myNum >= 0 to see if the slave successfully added
   * the stepper. If not, it's -1.
  */
  AccelStepperI2C(uint8_t i2c_address = 0x8,
                  uint8_t interface = AccelStepper::FULL4WIRE,
                  uint8_t pin1 = 2,
                  uint8_t pin2 = 3,
                  uint8_t pin3 = 4,
                  uint8_t pin4 = 5,
                  bool enable = true);

  //  AccelStepper(void (*forward)(), void (*backward)()); // constructor [2/2] makes no sense over I2C, the callbacks would have to go back from slave to master...
  void    moveTo(long absolute);
  void    move(long relative);
  
  /*!
   * @brief Don't use this, use state machine instead with runState().
   * @result If you use it for whatever reason, check sentOK and resultOK to be sure that things are alright and the return value can be trusted.
   */
  boolean run();

  /*!
   * @brief Don't use this, use state machine instead with runSpeedState().
   * @result If you use it for whatever reason, check sentOK and resultOK to be sure that things are alright and the return value can be trusted.
   */
  boolean runSpeed();

  void    setMaxSpeed(float speed);
  float   maxSpeed();
  void    setAcceleration(float acceleration);
  void    setSpeed(float speed);
  float   speed();
  long    distanceToGo();
  long    targetPosition();
  long    currentPosition();
  void    setCurrentPosition(long position);

  // void    runToPosition(); // blocking functions need to be implented on the master's side in the library, or not at all.

  /*!
   * @brief Don't use this, use state machine instead with runSpeedToPositionState().
   * @result If you use it for whatever reason, check sentOK and resultOK to be sure that things are alright and the return value can be trusted.
   */
  boolean runSpeedToPosition();

  // void    runToNewPosition(long position);  // blocking functions need to be implented on the master's side in the library, or not at all.

  void    stop();
  void    disableOutputs();
  void    enableOutputs();
  void    setMinPulseWidth(unsigned int minWidth);
  void    setEnablePin(uint8_t enablePin = 0xff);
  void    setPinsInverted(bool directionInvert = false, bool stepInvert = false, bool enableInvert = false);
  void    setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert);
  bool    isRunning();

  /*!
   * @brief Define global interrupt pin which can be enabled for each stepper 
   * seperately to inform the master that a state machine change occured.
   * @param pin Pin the slave will use to send out interrupts.
   * @param activeHigh If true, HIGH will signal an interrupt.
   * @sa enableInterrupts()
   */
  void setInterruptPin(int8_t pin, bool activeHigh = true);

  /*!
   * @brief Start or stop sending interrupts to master for this stepper. An 
   * interrupt will be sent
   * whenever a state machine change occured which was not triggered by the master.
   * At the moment, this could either be a target reached condition in runState()
   * or runSpeedToPositionState(), or an endstop reached condition.
   * @param enable true (default) to enable, false to disable.
   * @sa setInterruptPin()
   */
  void enableInterrupts(bool enable = true);
  
  /*!
   * @brief Define a new endstop pin. Each stepper can have up to two, so don't
   * call this more than twice per stepper.
   * @param pin Pin the endstop switch is connected to.
   * @param activeLow True if activated switch will pull pin LOW, false if
   * HIGH.
   * @param internalPullup Use internal pullup for this pin.
   * @sa Use AccelStepperI2C::enableEndstops() to tell the state machine to actually
   * use the endstop(s). Use AccelStepperI2C::endstops() to read their currentl
   * state
   */
  void setEndstopPin(int8_t pin, bool activeLow, bool internalPullup);

  /*!
   * @brief Tell the state machine to check the endstops regularly. If two 
   * switches are used, it does not differentiate them. On a hit, it will 
   * simply revert to state_stopped, but do nothing else. The master can use 
   * endstops() to find out what endstop was hit.
   * @note Currently there is no function to leave the endstop's zone. So you'll
   * probably need to disable the endstops with enableEndstops(false), move the
   * stepper back until the endstop is not triggered anymore, and enableEndstops() 
   * again.
   * @todo Find solution for moving a stepper out of an endstop's zone.
   * @param enable true (default) to enable, false to disable.
   */
  void enableEndstops(bool enable = true);
  
  /*!
   * @brief Read current state of endstops
   * @returns One bit for each endstop, LSB is always the last addded endstop.
   *   Bits take activeLow setting in account, i.e. 0 for open, 1 for closed
   *   switch.
   */
  uint8_t endstops(); // returns endstop(s) states in bits 0 and 1

  
  /*!
   * @brief Turn on/off diagnostic speed logging.
   * @param enable true for enable, false for disable
   * @sa diagnostics()
   * @note Diagnostics are not specific to a single stepper but refer to the
   * slave as a whole. So it doesn't matter which stepper's diagnostics methods 
   * you use.
   */
  void enableDiagnostics(bool enable = true);
  
  /*!
   * @brief Get most recent diagnostics data
   * @param report where to put the data, preallocated struct of type 
   * AccelStepper::diagnosticsReport.
   * @sa enableDiagnostics()
   * @note Diagnostics are not specific to a single stepper but refer to the
   * slave as a whole. So it doesn't matter which stepper's diagnostics methods 
   * you use.
   */
  void diagnostics(diagnosticsReport* report);
  
 
  /*!
   * @brief Set the state machine's state manually.
   * @param newState one of state_stopped, state_run, state_runSpeed, or state_runSpeedToPosition.
   */
  void setState(uint8_t newState);

  /*!
   * @brief Read the state machine's state (it may have been changed by endstop or target reached condition).
   * @result one of state_stopped, state_run, state_runSpeed, or state_runSpeedToPosition.
   */
  uint8_t getState();

  /*!
  * @brief Will stop any of the above states, i.e. stop polling. It does nothing else, so the master is solely in command of target, speed, and other settings.
  */
  void stopState();

  /*!
  * @brief Will poll run(), i.e. run to the target with acceleration and stop
     the state machine upon reaching it.
  */
  void runState();

  /*!
  * @brief Will poll runSpeed(), i.e. run at constant speed until told
  * otherwise (see AccelStepperI2C::stopState().
  */
  void runSpeedState();

  /*!
   * @brief Will poll runSpeedToPosition(), i.e. run at constant speed until target has been reached.
   */
  void runSpeedToPositionState();

  bool sentOK = true;   ///< True if previous function call was successfully transferred to slave.
  bool resultOK = true; ///< True if return value from previous function call was received successfully
  int8_t myNum = -1;    ///< Stepper number with myNum >= 0 for successfully added steppers.

 private:
  uint8_t addStepper(uint8_t interface = AccelStepper::FULL4WIRE, uint8_t pin1 = 2, uint8_t pin2 = 3, uint8_t pin3 = 4, uint8_t pin4 = 5, bool enable = true);
  void prepareCommand(uint8_t cmd);
  bool sendCommand();
  bool readResult(uint8_t numBytes);
  uint8_t address;
  SimpleBuffer buf;

};


#endif
