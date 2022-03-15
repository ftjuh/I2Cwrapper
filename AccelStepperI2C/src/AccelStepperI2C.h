/*!
  @file AccelStepperI2C.h
  @mainpage %AccelStepperI2C library
  @brief Arduino library for I2C-control of stepper motors connected to
  another Arduino which runs the associated @ref firmware.ino "AccelStepperI2C firmware".  
  See the @ref AccelStepperI2C "AccelStepperI2C class reference" for
  differences to the methods of the original AccelStepper class and for new
  methods of class %AccelStepperI2C. Functions which address not a single stepper but the slave as a
  whole are documented @ref I2Cwrapper "here (I2Cwrapper)".
  @section author Author
  Copyright (c) 2022 juh
  @section license License
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation, version 2.
  @todo add emergency stop/break pin for slave (just use reset pin for the moment)
  @todo ATM data is not protected against updates from ISRs while it is being
  used in the main program (see http://gammon.com.au/interrupts). Check if this
  could be a problem in our case.
  @todo ESP32: make use of dual cores?
  @todo use interrupts for endstops instead of main loop polling (not sure how
  much of a difference this would make in practice, though. The main loop is't
  doing much else, what really takes time are the computations.)
  @todo Master stop/start bit?
  @todo <del>update keywords.txt</del>
  @todo <del>clean up example sketches</del>
  @todo <del>implement runToPosition() and runToNewPosition() in master</del> - implemented
  @todo <del>test (and adapt) slave firmware for ESP8266</del> - implemented.
  However, I2C slave mode on ESP8266s is no fun. Run only with 160MHz CPU and
  start testing with 10kHz (that's right: ten kHz) I2C clock speed.
  @todo <del>checking each transmission with sentOK and resultOK is tedious. We could
  use some counter to accumulate errors and check them summarily, e.g. at
  the end of setup etc.</del> - AccelStepperI2C::sentErrors(),
  AccelStepperI2C::resultErrors() and AccelStepperI2C::transmissionErrors() added
  @todo <del>make time the slave has to answer I2C requests (I2CrequestDelay)
  configurable, as it will depend on µC and bus frequency etc.</del> -
  setI2Cdelay() implemented
  @todo <del>Versioning, I2C command to request version (important, as library
  and firmware always need to match)</del> - getVersion() and checkVersion() implemented
  @todo <del>Implement interrupt mechanism, so that the slave can inform the master
  about finished tasks or other events</del> - setInterruptPin() and enableInterrupts()
  implemented
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

// #define DEBUG // uncomment for serial debugging, don't forget Serial.begin() in your setup()

#include <Arduino.h>
#include <AccelStepper.h>
#include <I2Cwrapper.h>

#if !defined(log)
#if defined(DEBUG)
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG
#endif // log



const uint8_t AccelStepperI2CmaxBuf = 20; // upper limit of send and receive buffer(s)

// used as return valuefor calls with long/float result that got no correct reply from slave
// However, errors are now signaled with resultOK, so no need for a special value here, just take 0
const long resError = 0;


/*!
 * @brief Used to transmit diagnostic info with AccelStepperI2C::diagnostics().
 */
struct diagnosticsReport
{
  uint32_t cycles;          ///< Number of slave's main loop executions since the last reboot
  uint16_t lastProcessTime; ///< microseconds the slave needed to process (interpret) most recently received command
  uint16_t lastRequestTime; ///< microseconds the slave spent in the most recent onRequest() interrupt
  uint16_t lastReceiveTime; ///< microseconds the slave spent in the most recent onReceive() interrupt
};

// I2C commands and, if non void, returned bytes, for AccelStepper functions
const uint8_t moveToCmd             = 10;
const uint8_t moveCmd               = 11;
const uint8_t runCmd                = 12; const uint8_t runResult                = 1; // 1 boolean
const uint8_t runSpeedCmd           = 13; const uint8_t runSpeedResult           = 1; // 1 boolean
const uint8_t setMaxSpeedCmd        = 14;
const uint8_t maxSpeedCmd           = 15; const uint8_t maxSpeedResult           = 4; // 1 float
const uint8_t setAccelerationCmd    = 16;
const uint8_t setSpeedCmd           = 17;
const uint8_t speedCmd              = 18; const uint8_t speedResult              = 4; // 1 float
const uint8_t distanceToGoCmd       = 19; const uint8_t distanceToGoResult       = 4; // 1 long
const uint8_t targetPositionCmd     = 20; const uint8_t targetPositionResult     = 4; // 1 long
const uint8_t currentPositionCmd    = 21; const uint8_t currentPositionResult    = 4; // 1 long
const uint8_t setCurrentPositionCmd = 22;
const uint8_t runToPositionCmd      = 23; // blocking, implemented, but on master's side alone, so this code is unused
const uint8_t runSpeedToPositionCmd = 24; const uint8_t runSpeedToPositionResult = 1; // 1 boolean
const uint8_t runToNewPositionCmd   = 25; // blocking, implemented, but on master's side alone, so this code is unused
const uint8_t stopCmd               = 26;
const uint8_t disableOutputsCmd     = 27;
const uint8_t enableOutputsCmd      = 28;
const uint8_t setMinPulseWidthCmd   = 29;
const uint8_t setEnablePinCmd       = 30;
const uint8_t setPinsInverted1Cmd   = 31;
const uint8_t setPinsInverted2Cmd   = 32;
const uint8_t isRunningCmd          = 33; const uint8_t isRunningResult           = 1; // 1 boolean

// new commands for AccelStepperI2C start here

const uint8_t attachCmd             = 40; const uint8_t attachResult         = 1; // 1 uint8_t
const uint8_t enableDiagnosticsCmd  = 41;
const uint8_t diagnosticsCmd        = 42; const uint8_t diagnosticsResult        = sizeof(diagnosticsReport);
const uint8_t enableInterruptsCmd   = 43;
const uint8_t setStateCmd           = 44;
const uint8_t getStateCmd           = 45; const uint8_t getStateResult           = 1; // 1 uint8_t
const uint8_t setEndstopPinCmd      = 46;
const uint8_t enableEndstopsCmd     = 47;
const uint8_t endstopsCmd           = 48; const uint8_t endstopsResult           = 1; // 1 uint8_t


/// @brief stepper state machine states
const uint8_t state_stopped             = 0; ///< state machine is inactive, stepper can still be controlled directly
const uint8_t state_run                 = 1; ///< corresponds to AccelStepper::run(), will fall back to state_stopped if target reached or endstop hit
const uint8_t state_runSpeed            = 2; ///< corresponds to AccelStepper::runSpeed(), will remain active until stopped by user or endstop
const uint8_t state_runSpeedToPosition  = 3; ///< corresponds to AccelStepper::state_runSpeedToPosition(), will fall back to state_stopped if target position reached or endstop hit

/*!
 * @ingroup InterruptReasons
 * @{
 */
const uint8_t interruptReason_targetReachedByRun = 1;
const uint8_t interruptReason_targetReachedByRunSpeedToPosition = 2;
const uint8_t interruptReason_endstopHit = 3;
/*!
 * @}
 */


/*****************************************************************************/
/*****************************************************************************/

/*!
  @brief An I2C wrapper class for the [AccelStepper library](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html).
  @details
  This class mimicks the [original AccelStepper interface](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html).
  It replicates most of its methods and transmits each method call via I2C to
  a slave running the @ref firmware.ino "AccelStepperI2C firmware".
  Functions and parameters without documentation will work just as their original,
  but you need to take the general restrictions into account (e.g. don't take a return
  value for valid without error handling).
*/
class AccelStepperI2C
{
public:
  /*!
   * @brief Constructor.
   * @param w Wrapper object representing the slave the stepper is connected to.
   */
  AccelStepperI2C(I2Cwrapper* w);


  /*!
   * @brief Replaces the AccelStepper constructor and takes the same arguments.
   * Will allocate an AccelStepper object on the slave's side and make it ready for use.
   * @param interface Only AccelStepper::DRIVER is tested at the moment, but
   * AccelStepper::FULL2WIRE, AccelStepper::FULL3WIRE, AccelStepper::FULL4WIRE,
   * AccelStepper::HALF3WIRE, and AccelStepper::HALF4WIRE should work as well,
   * AccelStepper::FUNCTION of course not
   * @result Check @ref myNum >= 0 to see if the slave successfully added the
   * stepper. If not, it's -1.
   * @note The slave's platform pin names might not be known to the master's platform,
   * if both are different. So it is safer to use integer equivalents
   * as defined in the respective platform's `pins_arduino.h`, e.g. for ESP8266:
   * - static const uint8_t D0   = 16;
   * - static const uint8_t D1   = 5;
   * - static const uint8_t D2   = 4;
   * - static const uint8_t D3   = 0;
   * - static const uint8_t D4   = 2;
   * - static const uint8_t D5   = 14;
   * - static const uint8_t D6   = 12;
   * - static const uint8_t D7   = 13;
   * - static const uint8_t D8   = 15;
   * or for plain Arduinos:
   * - #define PIN_A0   (14)
   * - #define PIN_A1   (15)
   * - #define PIN_A2   (16)
   * - #define PIN_A3   (17)
   * - #define PIN_A4   (18)
   * - #define PIN_A5   (19)
   * - #define PIN_A6   (20)
   * - #define PIN_A7   (21)
   */
  void attach(uint8_t interface = AccelStepper::FULL4WIRE,
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

  /*!
   * @brief This is a blocking function in the original, it will only return
   * after the target has been reached. This I2C implementation mimicks the
   * original, but uses the state machine and checks for a target reached condition
   * with a fixed frequency of 100ms to keep the I2C bus uncluttered.
   * @note Does not check for endstops, implement your own loop if you need them.
   */
  void    runToPosition();

  /*!
   * @brief This is a blocking function in the original, it will only return
   * after the new target has been reached. This I2C implementation mimicks the
   * original, but uses the state machine and checks for a target reached condition
   * with a fixed frequency of 100ms to keep the I2C bus uncluttered.
   * @note Does not check for endstops, implement your own loop if you need them.
   */
  void    runToNewPosition(long position);

  /*!
   * @brief Don't use this, use state machine instead with runSpeedToPositionState().
   * @result If you use it for whatever reason, check sentOK and resultOK to be sure that things are alright and the return value can be trusted.
   */
  boolean runSpeedToPosition();


  void    stop();
  void    disableOutputs();
  void    enableOutputs();
  void    setMinPulseWidth(unsigned int minWidth);
  void    setEnablePin(uint8_t enablePin = 0xff);
  void    setPinsInverted(bool directionInvert = false, bool stepInvert = false, bool enableInvert = false);
  void    setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert);
  bool    isRunning();


  /*!
   * @brief Turn on/off diagnostic speed logging.
   * @param enable true for enable, false for disable
   * @sa diagnostics()
   */
  void enableDiagnostics(bool enable);


  /*!
   * @brief Get most recent diagnostics data. Needs diagnostics enabled and a slave
   * which was compiled with the @ref DIAGNOSTICS compiler
   * directive enabled.
   * @param report where to put the data, preallocated struct of type
   * diagnosticsReport.
   * @sa enableDiagnostics()
   */
  void diagnostics(diagnosticsReport* report);


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
   * @brief Tell the state machine to check the endstops regularly. If any of
   * the (one or two) stepper's endstops is hit, the state machine will  revert
   * to state_stopped. To prevent overshoot, also the stepper's speed is set to
   * 0 and its target to the current position.
   *
   * An interrupt will be triggered,
   * if interrupts are enabled, in that case I2Cwrapper::clearInterrupt() will
   * then inform about the interrupt's cause. Independent of an interrupt, the
   * master can use endstops() to find out if an endstop was hit and which one
   * it was.
   *
   * A "hit" condition is defined as an end stop switch becoming active.
   * Nothing happens if it stays active or becomes
   * inactive. A simple debouncing mechanism is in place. After a new flank
   * (switch becoming active *or* inactive), it will ignore any further flanks
   * for a set interval of 5 milliseconds, which should suffice for most
   * switches to get into a stable state.
   * @todo <del>Find solution for moving a stepper out of an endstop's zone.</del>
   * implemented debounce and active-flank detection so that there's no danger
   * of repeated interrupts being triggered while in an endstop's active zone.
   * @param enable true (default) to enable, false to disable.
   */
  void enableEndstops(bool enable = true);

  /*!
   * @brief Read current raw, i.e. not debounced state of endstops. It's basically 
   * one or two digitalReads() combined in one result.
   * @returns One bit for each endstop, LSB is always the last addded endstop.
   * Takes activeLow setting in account, i.e. an activated switch will always 
   * return 1.
   * @sa enableEndstops()
   */
  uint8_t endstops(); // returns endstop(s) states in bits 0 and 1


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

  int8_t myNum = -1;    ///< Stepper number with myNum >= 0 for successfully added steppers.

private:
  //uint8_t attach(uint8_t interface = AccelStepper::FULL4WIRE, uint8_t pin1 = 2, uint8_t pin2 = 3, uint8_t pin3 = 4, uint8_t pin4 = 5, bool enable = true);
  I2Cwrapper* wrapper;

};


#endif
