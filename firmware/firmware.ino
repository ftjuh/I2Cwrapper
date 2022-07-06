/*!
   @file firmware.ino
   @brief Generic firmware framework for I2C targets with modular functionality,
   built around the I2Cwrapper library
   ## Author
   Copyright (c) 2022 juh
   ## License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @todo <del>make I2C address configurable by hardware (with module?)</del>
   @todo return messages (results) should ideally come with an id, too, so that controller can be sure
      it's the correct result. Currently only CRC8, i.e. correct transmission is checked.
   @todo volatile variables / ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {}
   @todo Reduce memory use to make it fit into an <del>8k Attiny</del>4k ATtiny
*/

#define DEBUG // Uncomment this to enable library debugging output on Serial

#include <Arduino.h>
#include <Wire.h>
#include <I2Cwrapper.h>


/*
   Module framework stages. Used to include specific module sections at the right locations.
*/
#define MF_STAGE_includes       1
#define MF_STAGE_declarations   2
#define MF_STAGE_setup          3
#define MF_STAGE_loop           4
#define MF_STAGE_processMessage 5
#define MF_STAGE_reset          6
#define MF_STAGE_receiveEvent   7
#define MF_STAGE_requestEvent   8
#define MF_STAGE_I2CstateChange 9


/************************************************************************/
/************************************************************************/

uint8_t i2c_address = I2CwrapperDefaultAddress;  // this target's I2C address, will later be set by module or left to this default

/* !
  @brief [deprecated in v0.3.0, don't use] 
  Uncomment this to enable time keeping diagnostics. You probably should disable
  debugging, as Serial output will distort the measurements severely. Diagnostics
  take a little extra time and ressources, so you best disable it in production
  environments.
  @todo make diagnostics another module?
*/
//#define DIAGNOSTICS [deprecated in v0.3.0, don't use]




/*
   Inject module includes
*/
#define MF_STAGE MF_STAGE_includes
#include "firmware_modules.h"
#undef MF_STAGE


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
volatile uint8_t sentOnRequest = 0; // used by requestEvent() to signal main loop that buffer has been sent
uint32_t now, then = millis();
bool reportNow = true;
uint32_t lastCycles = 0; // for simple cycles/reportPeriod diagnostics
const uint32_t reportPeriod = 2000; // ms between main loop simple diagnostics output
// #define DEBUG_printCycles  // uncomment to print no. of cycles each reportPeriod
#endif // DEBUG



/*
   Diagnostics stuff [deprecated in v0.3.0, don't use]
*/

// #if defined(DIAGNOSTICS)
// bool diagnosticsEnabled = false;
// uint32_t thenMicros;
// diagnosticsReport currentDiagnostics;
// uint32_t previousLastReceiveTime; // used to delay receive times by one receive event
// #endif // DIAGNOSTICS

uint32_t cycles = 0; // keeps count of main loop iterations


/*
   I2C stuff
*/

SimpleBuffer* bufferIn;
SimpleBuffer* bufferOut;
volatile uint8_t newMessage = 0; // signals main loop that a receiveEvent with valid new data occurred

// I2C state machine: takes care that we don't end up in an undefined state if things get out of order,
// i.e. if an interrupt (receiveEvent or requestEvent) happens at an unexpected point in time
enum I2Cstates {
  initializing,         // setup not finished yet                               [==> readyForCommand]
  readyForCommand,      // expecting command by receiveEvent() in main loop()   [==> processingCommand]
  processingCommand,    // interpreting command in processMeassage() function   [==> readyForResponse *OR* readyForCommand]
  readyForResponse,     // expecting requestEvent() in main loop()              [==> responding]
  responding,           // sending bufferOut in writeOutputBuffer()             [==> readyForCommand]
  // Note that state "responding" should only be relevant for ESP32 which needs to write the buffer outside of the interrupt routine.
  // All other should be safe, as an interrupt cannot be interrupted (@todo or can it?)
  tainted               // bufferOut data is tainted and needs to be discarded  [==> readyForCommand]
};
volatile I2Cstates I2Cstate = initializing;

void changeI2CstateTo(I2Cstates newState) {
  I2Cstate = newState;
#if defined(DEBUG)
  log("   * Switched I2C state to '");
  switch (newState) {
    case initializing:
      log("initializing'\n");
      break;
    case readyForCommand:
      log("readyForCommand'\n");
      break;
    case processingCommand:
      log("processingCommand'\n");
      break;
    case readyForResponse:
      log("readyForResponse'\n");
      break;
    case responding:
      log("responding'\n");
      break;
    case tainted:
      log("tainted'\n");
      break;
  }
#endif
  /*
     Inject module code for I2C state change
  */
#define MF_STAGE MF_STAGE_I2CstateChange
#include "firmware_modules.h"
#undef MF_STAGE
}


// Forward declarations. Without them, the Arduino magic will be confused by the IRAM_ATTR stuff.
// Not sure if the IRAM stuff needs to happen here already, but I guess it won't harm.
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) // both platforms now use "IRAM_ATTR"
void IRAM_ATTR receiveEvent(int howMany);
void IRAM_ATTR requestEvent();
#else
void receiveEvent(int howMany);
void requestEvent();
#endif


/*
   Interrupt (to controller) stuff
   note: The interrupt pin is global, as there is only one shared by all modules
   and units. However, modules can implement them so that interrupts can be
   enabled for each unit seperately,
   Note: these variables are initialized during setup() in initializeFirmware().
*/

int8_t interruptPin;
bool interruptActiveHigh;
uint8_t interruptSource;
uint8_t interruptReason;

/*!
   @brief Interrupt controller if an interrupt pin has been set and, optionally,
   define a source and reason having caused the interrupt that the controller can
   learn about by calling the I2Cwrapper::clearInterrupt() function.
   @param source 4-bit value that can be used to signal the controller where in the
   target device the interrupt occured, e.g. which end stop switch was triggered
   or which touch button was touched. 0xF is reserved to signal "unknown source"
   @param reason 4-bit value that can be used to differentiate between different
   interrupt causing events, e.g. end stop hit vs. target reached. 0xF is
   reserved to signal "unknown reason"
*/
void triggerInterrupt(uint8_t source, uint8_t reason)
{
  if (interruptPin >= 0) {
    //log("~~ interrupt controller with source = "); log(source);
    //log(" and reason = "); log(reason); log("\n");
    digitalWrite(interruptPin, interruptActiveHigh ? HIGH : LOW);
    interruptSource = source;
    interruptReason = reason;
  }
}

/*
   Called by command interpreter, no need for modules to call it.
*/
void clearInterrupt()
{
  if (interruptPin >= 0) {
    digitalWrite(interruptPin, interruptActiveHigh ? LOW : HIGH);
  }
}



/*
    Reset/initialization stuff
*/

// Put core firmware and modules (back) to initial state. This is used by setup() and resetCmd
void initializeFirmware()
{

  // reset/initialize interrupt subsystem
  clearInterrupt();
  interruptPin = -1; // -1 for undefined
  interruptActiveHigh = true;
  interruptSource = 0xF;
  interruptReason = interruptReason_none;

  // empty buffers
  bufferIn->reset();
  bufferOut->reset();

}


/*
   Inject module declarations
*/

#define MF_STAGE MF_STAGE_declarations
#include "firmware_modules.h"
#undef MF_STAGE


// Join I2C bus as target and start the necessary ISRs.
// This is outsourced to a function, as it is needed by setup() and reset code.
// Must be placed here after the MF_STAGE_declarations module injection, so that
// modules can claim I2C address definition by defining I2C_ADDRESS_DEFINED_BY_MODULE.
void startI2C() {

#ifdef I2C_ADDRESS_DEFINED_BY_MODULE // a module has defined a way to retrieve our address
  i2c_address = I2C_ADDRESS_DEFINED_BY_MODULE ;
#endif // otherwise leave the default address 

  if ((i2c_address < 0x08) or (i2c_address > 0x77)) { // prevent invalid address, see https://learn.adafruit.com/i2c-addresses/the-list
    i2c_address = I2CwrapperDefaultAddress;
  }

  Wire.begin(i2c_address);
  log("I2C started, listening to address "); log(i2c_address); log("\n\n");
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

}


/**************************************************************************/
/*!
    @brief Setup system.
*/
/**************************************************************************/
void setup()
{

#if defined(DEBUG)
  Serial.begin(115200);
#endif
  log("\n\n\n=== I2Cwrapper firmware v");
  log(I2Cw_VersionMajor); log("."); log(I2Cw_VersionMinor); log("."); log(I2Cw_VersionPatch); log(" ===\n");
  log("Running on architecture ");
#if defined(ARDUINO_ARCH_AVR)
  log("ARDUINO_ARCH_AVR\n");
#elif defined(ARDUINO_ARCbH_ESP8266)
  log("ARDUINO_ARCH_ESP8266\n");
#elif defined(ARDUINO_ARCH_ESP32)
  log("ARDUINO_ARCH_ESP32\n");
#else
  log("unknown\n");
#endif
  log("Compiled on " __DATE__ " at " __TIME__ "\n");

  /*
     Inject module setup()s
  */
#define MF_STAGE MF_STAGE_setup
#include "firmware_modules.h"
#undef MF_STAGE


  bufferIn = new SimpleBuffer; bufferIn->init(I2CmaxBuf);
  bufferOut = new SimpleBuffer; bufferOut->init(I2CmaxBuf);

  startI2C();

  initializeFirmware();

  changeI2CstateTo(readyForCommand);

}


/**************************************************************************/
/*!
    @brief Main loop. By default, it doesn't do a lot apart from debugging: It
    just checks if an ISR signaled that a new message has arrived and calls
    processMessage() accordingly.
*/
/**************************************************************************/

void loop()
{

#if defined(DEBUG)
  now = millis();
  if (now > then) { // report cycles/reportPeriod statistics
    reportNow = true;
#if defined(DEBUG_printCycles)
    log("\n    > Cycles/s = "); log((cycles - lastCycles) * 1000 / reportPeriod); log("\n");
#endif // defined(DEBUG_printCycles)
    lastCycles = cycles;
    then = now + reportPeriod;
  }
#endif // defined(DEBUG)

  /*
    Inject modules' loop section
  */
#define MF_STAGE MF_STAGE_loop
#include "firmware_modules.h"
#undef MF_STAGE

#if defined(DEBUG)
  if (reportNow) {
    log("\n");
    reportNow = false;
  }
#endif

  // Check for new incoming messages from I2C interrupt
  //if (newMessage > 0) {
  if (I2Cstate == processingCommand) {
    processMessage(newMessage);
  }



#if defined(DEBUG)
  if (sentOnRequest > 0) { // check if a requestEvent() happened and data was sent
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



// ================================================================================
// =========================== processMessage() ===================================
// ================================================================================

/**************************************************************************/
/*!
  @brief Process the message stored in bufferIn with lenght len
  which was received via I2C. Messages consist of at least three bytes:
  [0] CRC8 checksum
  [1] Command. Most commands correspond to a module's function, some
      are specific for I2Cwrapper core functions. Unknown commands are ignored.
  [2] Number of the  unit (stepper, servo, ...) that is to receive the command.
      Not all modules use units and ignore this value accordingly.
  [3..maxBuffer] Optional parameter bytes. Each command that comes with too
      many or too few parameter bytes is ignored.
*/
/**************************************************************************/
void processMessage(uint8_t len)
{

// #if defined(DIAGNOSTICS)
//   uint32_t thenMicrosP = micros();
// #endif // DIAGNOSTICS

  log("New message with "); log(len); log(" bytes. ");
  for (int j = 0; j < len; j++)  { // I expect the compiler will optimize this away if debugging is off
    log (bufferIn->buffer[j]); log(" ");
  }
  log("\n");

  if (bufferIn->checkCRC8()) {  // ignore invalid message ### how to warn the controller?

    bufferIn->reset(); // set reader to beginning of message
    uint8_t cmd; bufferIn->read(cmd);
    int8_t unit; bufferIn->read(unit); // stepper/servo/etc. addressed (if any, will not be used for general commands)
    int8_t i = len - 3 ; // bytes received minus 3 header bytes = number of parameter bytes
    log("CRC8 ok. Command = "); log(cmd); log(" for unit "); log(unit);
    log(" with "); log(i); log(" parameter bytes --> ");
    bufferOut->reset();  // let's hope last result was already requested by controller, as now it's gone

    switch (cmd) {

        /*
          Inject modules' processMessage sections
        */

#define MF_STAGE MF_STAGE_processMessage
#include "firmware_modules.h"
#undef MF_STAGE


      /*
         I2Cwrapper commands
      */

      case resetCmd: {
          if (i == 0) { // no parameters
            log("\n\n---> Resetting firmware and modules to initial state\n\n");
            changeI2CstateTo(initializing); // ignore interrupts during reset

            // Inject modules' reset code first
#define MF_STAGE MF_STAGE_reset
#include "firmware_modules.h"
#undef MF_STAGE

            initializeFirmware(); // then reset firmware to initial state

            // restart Wire and reread our current I2C address
#ifndef WIRE_HAS_END
#error "Sorry, the Wire library of your platform has no Wire.end() implementation"
#endif
            Wire.end();
            startI2C(); // will use updated I2C address, if it has been changed

#if defined(DEBUG)
            Serial.flush();
#endif
            // changeI2CstateTo(readyForCommand); // ready again // no, will be changed at the end of processMessage()
          }
        }
        break;

      // case changeI2CaddressCmd: { // now outsourced to _addressFromFlash_firmware.h

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

      case pingBackCmd: { // has variable amount of parameter bytes
          if (i >= 1) { // 1 uint8_t (testLength)
            uint8_t testLength; bufferIn->read(testLength);
            // test for i == 1 + testLength here?
            uint8_t receivedData;
            for (int i = 0; i < testLength; i++) {
              bufferIn->read(receivedData);
              bufferOut->write(receivedData);
            }
          }
        }
        break;

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
    // ESP32 target implementation needs the I2C-reply buffer prefilled. I.e. onRequest() will, without our own doing,
    // just send what's in the I2C buffer at the time of the request, and anything that's written during the request
    // will only be sent in the next request cycle. So let's prefill the buffer here.
    // ### what exactly is the role of slaveWrite() vs. Write(), here?
    // ### slaveWrite() is only for ESP32, not for it's poorer cousins ESP32-S2 and ESP32-C3. Need to fine tune the compiler directive, here?
    // log("   ESP32 buffer prefill  ");
    changeI2CstateTo(responding);
    writeOutputBuffer();
    changeI2CstateTo(readyForCommand);
#endif  // ESP32

  } // if (bufferIn->checkCRC8())
  log("\n");
  newMessage = 0;


  // determine new state
  if (I2Cstate != tainted) {
    if (bufferOut->idx > 1) {  // a reply is waiting to be requested; @todo buffer class needs a method to tell us if it's filled, this way is prone to error
      changeI2CstateTo(readyForResponse);
    } else { // we have no reply to give, so expect next command
      changeI2CstateTo(readyForCommand);
    }
  } else { // some unexpected interrupt made a mess, discard output and start again
    bufferOut->reset();  // discard output buffer
    changeI2CstateTo(readyForCommand);
  }


// #if defined(DIAGNOSTICS)
//   currentDiagnostics.lastProcessTime = micros() - thenMicrosP;
// #endif // DIAGNOSTICS

}


// ================================================================================
// ============================ receiveEvent() ====================================
// ================================================================================

/**************************************************************************/
/*!
  @ brief Handle I2C receive event. Just read the message and inform main loop.
*/
/**************************************************************************/

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) // both platforms now use "IRAM_ATTR"
void IRAM_ATTR receiveEvent(int howMany)
#else
void receiveEvent(int howMany)
#endif
{

// #if defined(DIAGNOSTICS)
//   thenMicros = micros();
// #endif // DIAGNOSTICS


  /*
     Inject module receiveEvent() code
  */
#define MF_STAGE MF_STAGE_receiveEvent
#include "firmware_modules.h"
#undef MF_STAGE

  switch (I2Cstate) {


    case readyForResponse: { // not to be expected here (this is why we should move it to the back, but it needs to be here up front for the fallthrough)
        /* An outputBuffer is ready for response, but the controller is sending another command instead of requesting
           its contents, making the buffer useless. So discard it right away, switch to readyForCommand state and
           fall through to receiving it. */
        bufferOut->reset();
        changeI2CstateTo(readyForCommand); // not really needed for fall through, but feels cleaner
      }
      [[fallthrough]];


    case readyForCommand:  { // this is the expected state when a receiveEvent happens

        //log("[Int with "); log(howMany); log(newMessage == 0 ? ", ready]\n" : ", NOT ready]\n");

        // if (newMessage == 0) { // Only accept message if an earlier one is fully processed. // this check obsolete with I2Cstate machine
        bufferIn->reset();
        for (uint8_t i = 0; i < howMany; i++) {
          bufferIn->buffer[i] = Wire.read();
        }
        bufferIn->idx = howMany;
        newMessage = howMany; // tell main loop that and how much data has arrived
        changeI2CstateTo(processingCommand);  // and move on to next state
        // }

      } // case readyForCommand
      break;


    case processingCommand: { // not to be expected here
        /* An earlier command is still beeing processed, so the outputBuffer is possibly being filled right now,
          but the controller is already sending another command, making the buffer useless.
          We cannot terminate the processMessage() function from here, so signal it that it should discard the
          buffer at the end of processing and switch to readyForCommand state after. */
        changeI2CstateTo(tainted);
      }
      break;


    case initializing:
    case responding:
    case tainted:
      break; // do nothing, ignore receiveEvent and stay in the respective state

  } // switch (I2Cstate)

// #if defined(DIAGNOSTICS)
  // delay storing the executing time for one cycle, else diagnostics() would always return
  // its own receive time, not the one of the previous command
//   currentDiagnostics.lastReceiveTime = previousLastReceiveTime;
//   previousLastReceiveTime = micros() - thenMicros;
// #endif // DIAGNOSTICS

}



// ================================================================================
// ========================== writeOutputBuffer() =================================
// ================================================================================

// If there is anything in the output buffer, write it out to I2C.
// This is outsourced to a function, as, depending on the architecture,
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


// ================================================================================
// ============================ requestEvent() ====================================
// ================================================================================
/*!
  @brief Handle I2C request event. Will send results or information requested
    by the last command, as defined by the contents of the outputBuffer.
  @todo <del>Find sth. meaningful to report from requestEvent() if no message is
  pending, e.g. current position etc.</del> not implemented, ESP32 can't do that (doh)

*/

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
void IRAM_ATTR requestEvent()
#else
void requestEvent()
#endif
{

// #if defined(DIAGNOSTICS)
//   thenMicros = micros();
// #endif // DIAGNOSTICS


  /*
     Inject module receiveEvent() code
  */
#define MF_STAGE MF_STAGE_requestEvent
#include "firmware_modules.h"
#undef MF_STAGE

  switch (I2Cstate) {


    case readyForResponse: {

        changeI2CstateTo(responding);

#if !defined(ARDUINO_ARCH_ESP32) // ESP32 has (hopefully) already written the buffer in the main loop
        writeOutputBuffer();
#endif // not ESP32

#if defined(DEBUG)
        sentOnRequest = writtenToBuffer; // signal main loop that we sent buffer contents
#endif // DEBUG

        changeI2CstateTo(readyForCommand);

      } // case readyForResponse
      break;

    case processingCommand: {
        /* A command is still beeing processed and the outputBuffer is not ready yet, the Controller is
          probably too eager to want its reply. Unfortunately, an incomplete buffer is useless.
          We cannot terminate the processMessage() function from here, so signal it that it should discard the
          buffer at the end of processing and switch to readyForCommand state after. */
        changeI2CstateTo(tainted);
      }
      break;

    case readyForCommand:
    case initializing:
    case responding:
    case tainted:
      break; // do nothing, ignore requestEvent and stay in the respective state

  } // switch (I2Cstate)

// #if defined(DIAGNOSTICS)
//   currentDiagnostics.lastRequestTime = micros() - thenMicros;
// #endif // DIAGNOSTICS

}
