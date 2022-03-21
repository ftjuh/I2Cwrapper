#/*!
   @file firmware.ino
   @brief Firmware for an I2C slave controlling up to 8 stepper motors with the AccelStepper library,
   comes with the matching AccelStepperI2C library.
   @details This documentation is probably only relevant for you if you want to change the firmware.
   Otherwise, the library documentation should suffice.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @todo return messages (results) should ideally come with an id, too, so that master can be sure
      it's the correct result. Currently only CRC8, i.e. correct transmission is checked.
   @todo volatile variables / ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {}
   @todo Module support (Servo, pin control, ...) needs to be outsourced and centralized. Currently
   there are a number of places that need to be adapted if a new module is added: Init section, startup
   message in setup(), processMessage().
   @todo <del>make i2c address configurable with pins or via i2c/EEPROM</del>
   @todo <del>implement endstops/reference positions</del>
*/

//#define DEBUG // Uncomment this to enable library debugging output on Serial

#include <Arduino.h>
#include <Wire.h>
#include <AccelStepperI2C.h>
#include <EEPROM.h>
// note: optional libraries like ServoI2C.h are conditionally included further below

//#if defined(ARDUINO_ARCH_AVR) not needed anymore, reset now without watchdog
//#include <avr/wdt.h>
//#endif


/************************************************************************/
/************* firmware configuration settings **************************/
/************************************************************************/

/*
   I2C address
   Change this if you want to flash the firmware with a different I2C address.
   Alternatively, you can use setI2Caddress() from the library to change it later
*/
const uint8_t slaveDefaultAddress = 0x08; // default

/*
   uncomment and define max. number of servos
   to enable servo support
*/
#define SERVO_SUPPORT
const uint8_t maxServos = 4;


/*
   uncomment to enable pin control support
*/
#define PINCONTROL_SUPPORT


/*!
  @brief Uncomment this to enable time keeping diagnostics. You probably should disable
  debugging, as Serial output will distort the measurements severely. Diagnostics
  take a little extra time and ressources, so you best disable it in production
  environments.
*/
//#define DIAGNOSTICS

/*!
  @brief Uncomment this to enable firmware debugging output on Serial.
*/
#define DEBUG

/************************************************************************/
/******* end of firmware configuration settings **************************/
/************************************************************************/

/*
   Servo stuff
*/

#if defined(SERVO_SUPPORT)

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_ESP8266)
#include <Servo.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <ESP32Servo.h> // ESP32 doesn't come with a native Servo.h
#endif // defined(ARDUINO_ARCH_AVR)
#include <ServoI2C.h>

Servo servos[maxServos]; // ### really allocate all of them now?
uint8_t numServos = 0; // number of initialised servos.

#endif // defined(SERVO_SUPPORT)

/*
   Pin control stuff
*/

#if defined(PINCONTROL_SUPPORT)
#include <PinI2C.h>
#endif // defined(PINCONTROL_SUPPORT)


/*
   Debugging stuff
*/

#if defined(DEBUG)
#undef log
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif // DEBUG

#if defined(DEBUG)
volatile uint8_t writtenToBuffer = 0;
volatile uint8_t sentOnRequest = 0;
uint32_t now, then = millis();
bool reportNow = true;
uint32_t lastCycles = 0; // for simple cycles/reportPeriod diagnostics
const uint32_t reportPeriod = 2000; // ms between main loop simple diagnostics output
#endif // DEBUG


/*
   Diagnostics stuff
*/

#if defined(DIAGNOSTICS)
bool diagnosticsEnabled = false;
uint32_t thenMicros;
diagnosticsReport currentDiagnostics;
uint32_t previousLastReceiveTime; // used to delay receive times by one receive event
#endif // DIAGNOSTICS

uint32_t cycles = 0; // keeps count of main loop iterations


/*
   EEPROM stuff
*/

#define EEPROM_OFFSET_I2C_ADDRESS 0 // where in eeprom is the I2C address stored, if any? [1 CRC8 + 4 marker + 1 address = 6 bytes]
const uint32_t eepromI2CaddressMarker = 0x12C0ACCF; // arbitrary 32bit marker proving that next byte in EEPROM is in fact an I2C address
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const uint32_t eepromUsedSize = 6; // total bytes of EEPROM used by us
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)


/*
   I2C stuff
*/

SimpleBuffer* bufferIn;
SimpleBuffer* bufferOut;
volatile uint8_t newMessage = 0; // signals main loop that a receiveEvent with valid new data occurred


/*!
  @brief Read a stored I2C address from EEPROM. If there is none, use default.
*/
uint8_t retrieveI2C_address()
{
  SimpleBuffer b;
  b.init(8);
  // read 6 bytes from eeprom: [0]=CRC8; [1-4]=marker; [5]=address
  //log("Reading from EEPROM: ");
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.begin(256);
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  for (byte i = 0; i < 6; i++) {
    b.buffer[i] = EEPROM.read(EEPROM_OFFSET_I2C_ADDRESS + i);
    // log(b.buffer[i]); log (" ");
  }
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.end();
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("\n");
  // we wrote directly to the buffer, so reader idx is still at 1
  uint32_t markerTest; b.read(markerTest);
  uint8_t storedAddress; b.read(storedAddress); // now idx will be at 6, so that CRC check below will work
  if (b.checkCRC8() and (markerTest == eepromI2CaddressMarker)) {
    return storedAddress;
  } else {
    return slaveDefaultAddress;
  }
}


/*!
  @brief Write I2C address to EEPROM
  @note ESP eeprom.h works a bit different from AVRs: https://arduino-esp8266.readthedocs.io/en/3.0.2/libraries.html#eeprom
*/
void storeI2C_address(uint8_t newAddress)
{
  SimpleBuffer b;
  b.init(8);
  b.write(eepromI2CaddressMarker);
  b.write(newAddress);
  b.setCRC8();
  log("Writing to EEPROM: ");
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.begin(32);
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  for (byte i = 0; i < 6; i++) { // [0]=CRC8; [1-4]=marker; [5]=address
    EEPROM.write(EEPROM_OFFSET_I2C_ADDRESS + i, b.buffer[i]);
    //EEPROM.put(EEPROM_OFFSET_I2C_ADDRESS + i, b.buffer[i]);
    //delay(120);
    log(b.buffer[i]); log (" ");
  }
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.end(); // end() will also commit()
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("\n");
}


/*
   Interrupt (to master) stuff
   note: The interrupt pin is global, as there is only one shared by all steppers.
   However, interrupts can be enabled for each stepper seperately.
*/

int8_t interruptPin = -1; // -1 for undefined
bool interruptActiveHigh = true;
uint8_t interruptSource = 0xF;
uint8_t interruptReason = interruptReason_none;


/*
   Endstop stuff
*/

struct Endstop
{
  uint8_t pin;
  bool activeLow;
  //bool internalPullup; // we don't need to store this, we can directly use it when adding the pin
};
const uint8_t maxEndstops = 2; // not sure if there are scenarios where more than two make sense, but why not be prepared and make this configurable?
const uint32_t endstopDebouncePeriod = 5; // millisecends to keep between triggering endstop interrupts; I measured a couple of switches, none bounced longer than 1 ms so this should be more than safe


/*
   Stepper stuff
*/

const uint8_t maxSteppers = 8;
uint8_t numSteppers = 0; // number of initialised steppers

/*!
  @brief This struct comprises all stepper parameters needed for local slave management
*/
struct Stepper
{
  AccelStepper* stepper;
  uint8_t state = state_stopped;
  Endstop endstops[maxEndstops];
  uint8_t numEndstops = 0;
  bool interruptsEnabled = false;
  bool endstopsEnabled = false;
  uint8_t prevEndstopState; // needed for detecting rising and falling flanks
  uint32_t endstopDebounceEnd = 0; // used for debouncing, endstops are ignored after a new flank until this time is reached
};
Stepper steppers[maxSteppers];


/*
  Reset stuff
*/

void resetFunc()
{
#if defined(ARDUINO_ARCH_AVR)
  // The watchdog method supposedly is the preferred one, however it did not make my Nano restart normally,
  // I think maybe it got stuck in the bootloader.
  // cli();
  // wdt_enable(WDTO_30MS);
  // for (;;);
  // so let's just go for the stupid version, it should work fine for the firmware:
  asm volatile (" jmp 0");
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  ESP.restart();
#endif
}


/**************************************************************************/
/*!
    @brief Setup system. Retrieve I2C address from EEPROM or default
    and initialize I2C slave.
*/
/**************************************************************************/
void setup()
{

#if defined(DEBUG)
  Serial.begin(115200);
#endif
  log("\n\n\n=== AccelStepperI2C v");
  log(I2Cw_VersionMajor); log("."); log(I2Cw_VersionMinor); log("."); log(I2Cw_VersionPatch); log(" ===\n");
  log("Running on architecture "); 
  #if defined(ARDUINO_ARCH_AVR)
  log("ARDUINO_ARCH_AVR\n");
  #elif defined(ARDUINO_ARCH_ESP8266)
  log("ARDUINO_ARCH_ESP8266\n");
  #elif defined(ARDUINO_ARCH_ESP32)
  log("ARDUINO_ARCH_ESP32\n");
  #else
  log("unknown\n");
  #endif
  log("Compiled on " __DATE__ " at " __TIME__ "\n");
  /* ### not a good solution; these module dependent things need to be outsourced, 
   *  so that we don't need to make adaptions at many places when a new module is added.   *  
   */
  #ifdef SERVO_SUPPORT
  log("Servo support enabled.\n");
  #endif
  #ifdef PINCONTROL_SUPPORT
  log("Pin control support enabled.\n");
  #endif

  uint8_t i2c_address = retrieveI2C_address();
  Wire.begin(i2c_address);
  log("I2C started, listening to address "); log(i2c_address); log("\n\n");
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  bufferIn = new SimpleBuffer; bufferIn->init(AccelStepperI2CmaxBuf);
  bufferOut = new SimpleBuffer; bufferOut->init(AccelStepperI2CmaxBuf);

}

/**************************************************************************/
/*!
    @brief Assign and initialize new stepper. Calls the
     <a href="https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#a3bc75bd6571b98a6177838ca81ac39ab">
      AccelStepper's[1/2] constructor</a>
    @returns internal number (0-7) of stepper assigned to new stepper, -1 for error
*/
/**************************************************************************/
int8_t addStepper(uint8_t interface = AccelStepper::FULL4WIRE,
                  uint8_t pin1 = 2,
                  uint8_t pin2 = 3,
                  uint8_t pin3 = 4,
                  uint8_t pin4 = 5,
                  bool enable = true)
{
  if (numSteppers < maxSteppers) {
    steppers[numSteppers].stepper = new AccelStepper(interface, pin1, pin2, pin3, pin4, enable);
    steppers[numSteppers].state = state_stopped;
    log("Add stepper with internal myNum = "); log(numSteppers); log("\n");
    return numSteppers++;
  } else {
    log("-- Too many steppers, failed to add new one\n");
    return -1;
  }
}


/*
   Read endstop(s) for stepper s.
   Returns bit pattern, last added endstop is LSB; 0 for inactive, 1 for active (taking activeLow in account)
   Returns 0x0 if no endstops set.
*/

uint8_t pollEndstops(uint8_t s)
{
  uint8_t res = 0;
  for (uint8_t i = 0; i < steppers[s].numEndstops; i++) {
    //log(i); log(" p");
    //log(digitalRead(steppers[s].endstops[i].pin));  log(" ");
    res = (res << 1) | (digitalRead(steppers[s].endstops[i].pin) ^ steppers[s].endstops[i].activeLow); // xor
  }
  //log(" res="); log(res);       log(" ");
  return res;
}


/*
   Interrupt master if interrupts are enabled for the source stepper.
*/

void triggerInterrupt(uint8_t source, uint8_t reason)
{
  if (steppers[source].interruptsEnabled and interruptPin >= 0) {
    log("~~ interrupt master with source = "); log(source);
    log(" and reason = "); log(reason); log("\n");
    digitalWrite(interruptPin, interruptActiveHigh ? HIGH : LOW);
    interruptSource = source;
    interruptReason = reason;
  }
}

void clearInterrupt()
{
  if (interruptPin >= 0) {
    digitalWrite(interruptPin, interruptActiveHigh ? LOW : HIGH);
  }
}

/**************************************************************************/
/*!
    @brief Main loop, implements the state machine. Will check for each
    stepper's state and do the appropriate polling (run() etc.) as needed.
    Also checks for incoming commands and passes them on for processing.
    @todo endstop polling: overhead to check if polling is needed
    (timeToCheckTheEndstops) might cost more than it saves, so maybe just check
    each cycle even if it might be much more often than needed.
*/
/**************************************************************************/

void loop()
{

#if defined(DEBUG)
  now = millis();
  if (now > then) { // report cycles/reportPeriod statistics and state machine states for all defined steppers
    reportNow = true;
    log("\n    > Cycles/s = "); log((cycles - lastCycles) * 1000 / reportPeriod);
    if (numSteppers > 0) {
      log("  | [Steppers]:states =");
    }
    lastCycles = cycles;
    then = now + reportPeriod;
  }
#endif

  for (uint8_t i = 0; i < numSteppers; i++) {  // cycle through all defined steppers

#if defined(DEBUG)
    if (reportNow) {
      log("  ["); log(i); log("]:"); log(steppers[i].state);
    }
#endif

    bool timeToCheckTheEndstops = false; // ###todo: change polling to pinchange interrupt
    // ### do we need this at all? Why not just poll each cycle? It doesn't take very long.
    switch (steppers[i].state) {

      case state_run: // boolean AccelStepper::run
        if (not steppers[i].stepper->run()) { // target reached?
          steppers[i].state = state_stopped;
          triggerInterrupt(i, interruptReason_targetReachedByRun);
        }
        timeToCheckTheEndstops = true; // we cannot tell if there was a step, so we'll have to check every time. (one more reason to do it with interrupts)
        break;

      case state_runSpeed:  // boolean AccelStepper::runSpeed
        timeToCheckTheEndstops = steppers[i].stepper->runSpeed(); // true if stepped
        break;

      case state_runSpeedToPosition:  // boolean AccelStepper::runSpeedToPosition
        timeToCheckTheEndstops = steppers[i].stepper->runSpeedToPosition();  // true if stepped
        if (steppers[i].stepper->distanceToGo() == 0) {
          // target reached, stop polling
          steppers[i].state = state_stopped;
          triggerInterrupt(i, interruptReason_targetReachedByRunSpeedToPosition);
        }
        break;

      case state_stopped: // do nothing
        break;
    } // switch

#if defined(DEBUG)
    if (reportNow) {
      log("\n\n");
      reportNow = false;
    }
#endif

    if (timeToCheckTheEndstops and steppers[i].endstopsEnabled) { // the stepper (potentially) stepped a step, so let's look at the endstops
      uint8_t es = pollEndstops(i);
      if (es != steppers[i].prevEndstopState) { // detect rising *or* falling flank
        uint32_t ms = millis();
        if (ms > steppers[i].endstopDebounceEnd) { // primitive debounce: ignore endstops for some ms after each new flank
          log("** es: flank detected  \n");
          steppers[i].endstopDebounceEnd = ms + endstopDebouncePeriod; // set end of debounce period
          steppers[i].prevEndstopState = es;
          if (es != 0) { // this is a non-bounce, *rising* flank
            log("** es: endstop detected!\n");
            //steppers[i].stepper->stop();
            steppers[i].stepper->setSpeed(0);
            steppers[i].stepper->moveTo(steppers[i].stepper->currentPosition());
            steppers[i].state = state_stopped; // endstop reached, stop polling
            triggerInterrupt(i, interruptReason_endstopHit);
          }
        }
      }
    } // check endstops

  } // for

  // Check for new incoming messages from I2C interrupt
  if (newMessage > 0) {
    processMessage(newMessage);
    newMessage = 0;
  }

#if defined(DEBUG)
  if (sentOnRequest > 0) {
    log("Output buffer sent ("); log(sentOnRequest); log(" bytes): ");
    for (uint8_t j = 0; j < sentOnRequest; j++) {
      log(bufferOut->buffer[j]);  log(" ");
    }
    log("\n");
    sentOnRequest = 0;
  }
#endif

  cycles++;
}



/**************************************************************************/
/*!
  @brief Handle I2C receive event. Just read the message and inform main loop.
  @todo <del>Just read the buffer here, move interpretation out of the interrupt.</del>
*/
/**************************************************************************/
#if defined(ARDUINO_ARCH_ESP8266)
ICACHE_RAM_ATTR
#endif
void receiveEvent(int howMany)
{
  //log("[Int with "); log(howMany); log(newMessage == 0 ? ", ready]\n" : ", NOT ready]\n");
#if defined(DIAGNOSTICS)
  thenMicros = micros();
#endif // DIAGNOSTICS
  if (newMessage == 0) { // Only accept message if an earlier one is fully processed.
    bufferIn->reset();
    for (uint8_t i = 0; i < howMany; i++) {
      bufferIn->buffer[i] = Wire.read();
    }
    bufferIn->idx = howMany;
    newMessage = howMany; // tell main loop that new data has arrived
  }

#if defined(DIAGNOSTICS)
  // delay storing the executing time for one cycle, else diagnostics() would always return
  // its own receive time, not the one of the previous command
  currentDiagnostics.lastReceiveTime = previousLastReceiveTime;
  previousLastReceiveTime = micros() - thenMicros;
#endif // DIAGNOSTICS

}

bool validStepper(int8_t s)
{
  return (s >= 0) and (s < numSteppers);
}

bool validServo(int8_t s)
{
  return (s >= 0) and (s < numServos);
}

/**************************************************************************/
/*!
  @ brief Process the message stored in bufferIn with lenght len
  which was received via I2C. Messages consist of at least three bytes:
  [0] CRC8 checksum
  [1] Command. Most commands correspond to an AccelStepper or Servo function, some
      are new for I2C reasons. Unknown commands are ignored.
  [2] Number of the stepper or servo unit that is to receive the command. If no unit with
      this number exists, the message is ignored, except for general wrapper commands like
      resetCmd, which ignore this parameter.
  [3..maxBuffer] Optional parameter bytes. Each command that comes with too
      many or too few parameter bytes is ignored.
   @note Blocking functions runToPositionCmd() and runToNewPositionCmd() not
      implemented here. Blocking at the slave's side seems not too useful, as
      the master won't wait for us anyway,
    @todo Implement blocking functions runToPositionCmd() and runToNewPositionCmd()
    on the master's side/in the library?
*/
/**************************************************************************/
void processMessage(uint8_t len)
{

#if defined(DIAGNOSTICS)
  uint32_t thenMicrosP = micros();
#endif // DIAGNOSTICS

  log("New message with "); log(len); log(" bytes. ");
  for (int j = 0; j < len; j++)  { // I expect the compiler will optimize this away if debugging is off
    log (bufferIn->buffer[j]); log(" ");
  }
  log("\n");

  if (bufferIn->checkCRC8()) {  // ignore invalid message ### how to warn the master?

    bufferIn->reset(); // set reader to beginning of message
    uint8_t cmd; bufferIn->read(cmd);
    int8_t unit; bufferIn->read(unit); // stepper/servo/etc. addressed (if any, will not be used for general commands)
    int8_t i = len - 3 ; // bytes received minus 3 header bytes = number of parameter bytes
    log("CRC8 ok. Command = "); log(cmd); log(" for unit "); log(unit);
    log(" with "); log(i); log(" parameter bytes --> ");
    bufferOut->reset();  // let's hope last result was already requested by master, as now it's gone

    switch (cmd) {

      /*
       * AccelStepper commands
       */

      case moveToCmd: { // void   moveTo (long absolute)
          if (validStepper(unit) and (i == 4)) { // 1 long parameter (not nice to have these constants hardcoded here, but what the heck)
            long l = 0;
            bufferIn->read(l);
            steppers[unit].stepper->moveTo(l);
          }
        }
        break;

      case moveCmd: { // void   move (long relative)
          if (validStepper(unit) and (i == 4)) { // 1 long parameter
            long l = 0;
            bufferIn->read(l);
            steppers[unit].stepper->move(l);
          }
        }
        break;

      // usually not to be called directly via I2C, use state machine instead
      case runCmd: { // boolean  run ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            bool res = steppers[unit].stepper->run();
            bufferOut->write(res);
          }
        }
        break;

      // usually not to be called directly via I2C, use state machine instead
      case runSpeedCmd: { //  boolean   runSpeed ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            bool res = steppers[unit].stepper->runSpeed();
            bufferOut->write(res);
          }
        }
        break;

      case setMaxSpeedCmd: { // void   setMaxSpeed (float speed)
          if (validStepper(unit) and (i == 4)) { // 1 long parameter
            float f = 0;
            bufferIn->read(f);
            steppers[unit].stepper->setMaxSpeed(f);
          }
        }
        break;

      case maxSpeedCmd: { // float  maxSpeed ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            float f = steppers[unit].stepper->maxSpeed();
            bufferOut->write(f);
          }
        }
        break;

      case setAccelerationCmd: { // void   setAcceleration (float acceleration)
          if (validStepper(unit) and (i == 4)) { // 1 float parameter
            float f = 0;
            bufferIn->read(f);
            steppers[unit].stepper->setAcceleration(f);
          }
        }
        break;

      case setSpeedCmd: { // void   setSpeed (float speed)
          if (validStepper(unit) and (i == 4)) { // 1 float parameter
            float f = 0;
            bufferIn->read(f);
            steppers[unit].stepper->setSpeed(f);
          }
        }
        break;

      case speedCmd: { // float  speed ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            float f = steppers[unit].stepper->speed();
            bufferOut->write(f);
          }
        }
        break;

      case distanceToGoCmd: { // long   distanceToGo ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            long l = steppers[unit].stepper->distanceToGo();
            bufferOut->write(l);
          }
        }
        break;

      case targetPositionCmd: { // long   targetPosition ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            long l = steppers[unit].stepper->targetPosition();
            bufferOut->write(l);
          }
        }
        break;

      case currentPositionCmd: { // long   currentPosition ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            long l = steppers[unit].stepper->currentPosition();
            bufferOut->write(l);
          }
        }
        break;

      case setCurrentPositionCmd: { // void   setCurrentPosition (long position)
          if (validStepper(unit) and (i == 4)) { // 1 long parameter
            long l = 0;
            bufferIn->read(l);
            steppers[unit].stepper->setCurrentPosition(l);
          }
        }
        break;

      // blocking, implemented in master library
      // case runToPositionCmd:  // void   runToPosition () {}
      //  break;

      // usually not to be called directly via I2C, use state machine instead
      case runSpeedToPositionCmd: { // boolean  runSpeedToPosition ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            bool res = steppers[unit].stepper->runSpeedToPosition();
            bufferOut->write(res);
          }
        }
        break;

      // blocking, implemented in master library
      // case runToNewPositionCmd: // void   runToNewPosition (long position) {}
      //  break;

      case stopCmd: { // void   stop ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            steppers[unit].stepper->stop();
          }
        }
        break;

      case disableOutputsCmd: { // virtual void   disableOutputs ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            steppers[unit].stepper->disableOutputs();
          }
        }
        break;

      case enableOutputsCmd: { // virtual void   enableOutputs ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            steppers[unit].stepper->enableOutputs();
          }
        }
        break;

      case setMinPulseWidthCmd: { // void   setMinPulseWidth (unsigned int minWidth)
          if (validStepper(unit) and (i == 2)) {
            // 1 uint16
            uint16_t minW = 0;
            bufferIn->read(minW);
            steppers[unit].stepper->setMinPulseWidth(minW);
          }
        }
        break;

      case setEnablePinCmd: { // void   setEnablePin (uint8_t enablePin=0xff)
          if (validStepper(unit) and (i == 1)) {
            // 1 uint8_t
            uint8_t pin = 0;
            bufferIn->read(pin);
            steppers[unit].stepper->setEnablePin(pin);
          }
        }
        break;

      case setPinsInverted1Cmd: { // void   setPinsInverted (bool directionInvert=false, bool stepInvert=false, bool enableInvert=false)
          if (validStepper(unit) and (i == 1)) {
            // 8 bits
            uint8_t b = 0;
            bufferIn->read(b);
            steppers[unit].stepper->setPinsInverted(
              (b & 1 << 0) != 0, (b & 1 << 1) != 0, (b & 1 << 2) != 0);
          }
        }
        break;

      case setPinsInverted2Cmd: { //  void  setPinsInverted (bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert)
          if (validStepper(unit) and (i == 1)) {
            // 8 bits
            uint8_t b;
            bufferIn->read(b);
            steppers[unit].stepper->setPinsInverted(
              (b & 1 << 0) != 0, (b & 1 << 1) != 0, (b & 1 << 2) != 0, (b & 1 << 3) != 0, (b & 1 << 4) != 0);
          }
        }
        break;

      case isRunningCmd: { // bool   isRunning ()
          if (validStepper(unit) and (i == 0)) { // no parameters
            bool b = steppers[unit].stepper->isRunning();
            bufferOut->write(b);
          }
        }
        break;


      /*
          AccelstepperI2C state machine and other new commands
      */


      case attachCmd: { //
          //log("addStepperCmd\n");
          if (i == 6) { // 5 uint8_t + 1 bool
            uint8_t interface; bufferIn->read(interface);
            uint8_t pin1; bufferIn->read(pin1);
            uint8_t pin2; bufferIn->read(pin2);
            uint8_t pin3; bufferIn->read(pin3);
            uint8_t pin4; bufferIn->read(pin4);
            bool enable; bufferIn->read(enable);
            int8_t num = addStepper(interface, pin1, pin2, pin3, pin4, enable);
            bufferOut->write(num);
          }
        }
        break;

#if defined(DIAGNOSTICS)

      case enableDiagnosticsCmd: {
          if (i == 1) { // 1 bool
            bufferIn->read(diagnosticsEnabled);
          }
        }
        break;

      case diagnosticsCmd: {
          if (i == 0) { // no parameters
            currentDiagnostics.cycles = cycles;
            bufferOut->write(currentDiagnostics);
            cycles = 0;
          }
        }
        break;

#endif // DIAGNOSTICS

      case enableInterruptsCmd: { //
          if (validStepper(unit) and (i == 1)) { // 1 bool
            bufferIn->read(steppers[unit].interruptsEnabled);
          }
        }
        break;

      case setStateCmd: { //
          if (validStepper(unit) and (i == 1)) { // 1 uint8_t
            uint8_t newState;
            bufferIn->read(newState);
            steppers[unit].state = newState;
          }
        }
        break;


      case getStateCmd: { //
          if (validStepper(unit) and (i == 0)) { // no parameters
            bufferOut->write(steppers[unit].state);
          }
        }
        break;


      case setEndstopPinCmd: { //
          if (validStepper(unit) and (i == 3) and (steppers[unit].numEndstops < maxEndstops)) {
            int8_t pin; bufferIn->read(pin);
            bool activeLow; bufferIn->read(activeLow);
            bool internalPullup; bufferIn->read(internalPullup);
            steppers[unit].endstops[steppers[unit].numEndstops].pin = pin;
            steppers[unit].endstops[steppers[unit].numEndstops].activeLow = activeLow;
            pinMode(pin, internalPullup ? INPUT_PULLUP : INPUT);
            steppers[unit].numEndstops++;
          }
        }
        break;

      case enableEndstopsCmd: {
          if (validStepper(unit) and (i == 1)) { // 1 bool
            bool en; bufferIn->read(en);
            steppers[unit].endstopsEnabled = en;
            if (en) { // prevent that an interrupt is triggered immediately in case an endstop happens to be active at the moment
              steppers[unit].prevEndstopState = pollEndstops(unit);
            }
          }
        }
        break;

      case endstopsCmd: {
          if (validStepper(unit) and (i == 0)) { // no parameters
            uint8_t b = pollEndstops(unit);
            bufferOut->write(b);
          }
        }
        break;



      /*
         I2Cwrapper commands
      */


      case resetCmd: {
          if (i == 0) { // no parameters
            log("\n\n---> Resetting\n\n");
            for (uint8_t j = 0; j < numSteppers; j++) {
              steppers[j].stepper->stop();
              steppers[j].stepper->disableOutputs();
            }
          }
#if defined(DEBUG)
          Serial.flush();
#endif
          resetFunc();
        }
        break;

      case changeI2CaddressCmd: {
          if (i == 1) { // 1 uint8_t
            uint8_t newAddress; bufferIn->read(newAddress);
            log("Storing new Address "); log(newAddress); log("\n");
            storeI2C_address(newAddress);
          }
        }
        break;
        
      case setInterruptPinCmd: {
          if (i == 2) {
            bufferIn->read(interruptPin);
            bufferIn->read(interruptActiveHigh);
            pinMode(interruptPin, OUTPUT);
            clearInterrupt();
          }
        }
        break;

      case clearInterruptCmd: {
          if (i == 0) { // no parameters
            clearInterrupt(); // reset pin state
            bufferOut->write(uint8_t(interruptReason << 4 | interruptSource));
            interruptSource = 0xF;
            interruptReason = 0xF;
          }
        }
        break;

      case getVersionCmd: {
          if (i == 0) { // no parameters
            bufferOut->write(I2Cw_Version);
          }
        }
        break;


      /*
         ServoI2C commands
         
         Note: servo implementation is dumb. It passes each command over I2C even if only some of the commands 
         actually do something slave related. It would be much smarter to let the master do commands like 
         attached() without I2C traffic. The dumb way however is easier and safer.         
      */


#if defined(SERVO_SUPPORT)

      case servoAttach1Cmd: {
          if ((numServos < maxServos) and (i == 2)) { // 1 int
            int p; bufferIn->read(p);
            servos[numServos].attach(p);
            bufferOut->write(numServos++);
            log(numServos);
          }
        }
        break;

      case servoAttach2Cmd: {
          if ((numServos < maxServos) and (i == 6)) { // 3 int
            int p; bufferIn->read(p);
            int min; bufferIn->read(min);
            int max; bufferIn->read(max);
            servos[numServos].attach(p, min, max);
            bufferOut->write(numServos++);
          }
        }
        break;

      case servoDetachCmd: {
          if (validServo(unit) and (i == 0)) { // no parameters
            servos[unit].detach();
          }
        }
        break;

      case servoWriteCmd: {
          if (validServo(unit) and (i == 2)) { // 1 int
            int value; bufferIn->read(value);
            servos[unit].write(value);
          }
        }
        break;

      case servoWriteMicrosecondsCmd: {
          if (validServo(unit) and (i == 2)) { // 1 int
            int value; bufferIn->read(value);
            servos[unit].writeMicroseconds(value);
          }
        }
        break;

      case servoReadCmd: {
          if (validServo(unit) and (i == 0)) { // no parameters
            bufferOut->write(servos[unit].read());
          }
        }
        break;

      case servoReadMicrosecondsCmd: {
          if (validServo(unit) and (i == 0)) { // no parameters
            bufferOut->write(servos[unit].readMicroseconds());
          }
        }
        break;

      case servoAttachedCmd: {
          if (validServo(unit) and (i == 0)) { // no parameters
            bufferOut->write(servos[unit].attached());
          }
        }
        break;

#endif // defined(SERVO_SUPPORT)



      /*

         Pin control commands
         
      */

#if defined(PINCONTROL_SUPPORT)

      case pinPinModeCmd: {
          if (i == 2) { // 2 uint8_t
            uint8_t pin; bufferIn->read(pin);
            uint8_t mode; bufferIn->read(mode);
            log("pinMode("); log(pin); log(", "); log(mode); log(")\n\n");
            pinMode(pin, mode);
          }
        }
        break;

      case pinDigitalReadCmd: {
          if (i == 1) { // 1 uint8_t
            uint8_t pin; bufferIn->read(pin);            
            bufferOut->write((int16_t)digitalRead(pin)); // int is not 2 bytes on all Arduinos (sigh)
          }
        }
        break;

      case pinDigitalWriteCmd: {
          if (i == 2) { // 2 uint8_t
            uint8_t pin; bufferIn->read(pin);
            uint8_t value; bufferIn->read(value);
            digitalWrite(pin, value);
          }
        }
        break;

      case pinAnalogReadCmd: {
          if (i == 1) { // 1 uint8_t
            uint8_t pin; bufferIn->read(pin);            
            bufferOut->write((int16_t)analogRead(pin)); // int is not 2 bytes on all Arduinos (sigh)
          }
        }
        break;


      case pinAnalogWriteCmd: {
          if (i == 3) { // 1 uint8_t, 1 int16_t ("int")
            uint8_t pin; bufferIn->read(pin);
            int16_t value; bufferIn->read(value);
            analogWrite(pin, value);
          }
        }
        break;

#if defined(ARDUINO_ARCH_AVR) // no analogReference() on ESPs
      case pinAnalogReferenceCmd: {
          if (i == 1) { //1 uint8_t
            uint8_t mode; bufferIn->read(mode);
            analogReference(mode);
          }
        }
        break;
#endif // defined(ARDUINO_ARCH_AVR)

#endif // defined(PINCONTROL_SUPPORT)


      default:
        log("No matching command found");

    } // switch

#if defined(DEBUG)
    if (bufferOut->idx > 1) {
      log("Output buffer incl. CRC8 after processing ("); log(bufferOut->idx); log(" bytes): ");
      for (uint8_t i = 0; i < bufferOut->idx; i++) {
        log(bufferOut->buffer[i]);  log(" ");
      }
      log("\n");
    } // if (bufferOut->idx > 1)
#endif

#if defined(ARDUINO_ARCH_ESP32)
    // ESP32 slave implementation needs the I2C-reply buffer prefilled. I.e. onRequest() will, without our own doing,
    // just send what's in the I2C buffer at the time of the request, and anything that's written during the request
    // will only be sent in the next request cycle. So let's prefill the buffer here.
    // ### what exactly is the role of slaveWrite() vs. Write(), here?
    // ### slaveWrite() is only for ESP32, not for it's poorer cousins ESP32-S2 and ESP32-C3. Need to fine tune the compiler directive, here?
    log("   ESP32 buffer prefill  ");
    writeOutputBuffer();
#endif  // ESP32

  } // if (bufferIn->checkCRC8())
  log("\n");


#if defined(DIAGNOSTICS)
  currentDiagnostics.lastProcessTime = micros() - thenMicrosP;
#endif // DIAGNOSTICS

}

// If there is anything in the output buffer, write it out to I2C.
// This is outdourced to a function, as, depending on the architecture,
// it is called either directly from the interrupt (AVR) or from
// message processing (ESP32).
void writeOutputBuffer()
{

  if (bufferOut->idx > 1) {

    bufferOut->setCRC8();

#if defined(ARDUINO_ARCH_ESP32)
    Wire.slaveWrite(bufferOut->buffer, bufferOut->idx);
#else
    Wire.write(bufferOut->buffer, bufferOut->idx);
#endif  // ESP32

#if defined(DEBUG)
//    // for AVRs logging to Serial will happen in an interrupt - not recommenended but seems to work
//    log("sent "); log(bufferOut->idx); log(" bytes: ");
//    for (uint8_t i = 0; i < bufferOut->idx; i++) {
//      log(bufferOut->buffer[i]);  log(" ");
//    }
//    log("\n");
    writtenToBuffer = bufferOut->idx; // store this (for ESP32) to signal main loop later that we sent buffer
#endif

    bufferOut->reset();  // never send anything twice

  }
}


/**************************************************************************/
/*!
  @brief Handle I2C request event. Will send results or information requested
    by the last command, as defined by the contents of the outputBuffer.
  @todo <done>Find sth. meaningful to report from requestEvent() if no message is
  pending, e.g. current position etc.</done> not implemented, ESP32 can't do that (doh)
*/
/**************************************************************************/
#if defined(ARDUINO_ARCH_ESP8266)
ICACHE_RAM_ATTR
#endif
void requestEvent()
{

#if defined(DIAGNOSTICS)
  thenMicros = micros();
#endif // DIAGNOSTICS

#if !defined(ARDUINO_ARCH_ESP32) // ESP32 has (hopefully) already written the buffer in the main loop
  writeOutputBuffer();
#endif // not ESP32

#if defined(DEBUG)
  sentOnRequest = writtenToBuffer; // signal main loop that we sent buffer contents
#endif // DEBUG

#if defined(DIAGNOSTICS)
  currentDiagnostics.lastRequestTime = micros() - thenMicros;
#endif // DIAGNOSTICS

}
