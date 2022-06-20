/*!
   @file firmware.ino
   @brief Generic firmware framework for I2C targets with modular functionality,
   built around the I2Cwrapper library
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @todo make I2C address configurable by hardware (with module?)
   @todo return messages (results) should ideally come with an id, too, so that controller can be sure
      it's the correct result. Currently only CRC8, i.e. correct transmission is checked.
   @todo volatile variables / ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {}
   @todo Reduce memory use to make it fit into an 8k Attiny
*/

/*!
  @brief Uncomment this to enable firmware debugging output on Serial.
*/
//#define DEBUG

#include <Arduino.h>
#include <Wire.h>
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
#define USE_EEPROM
#if defined(ARDUINO_ARCH_SAMD)
#include <FlashAsEEPROM.h>
#else
#include <EEPROM.h>
#endif
#endif
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


/************************************************************************/
/************* firmware configuration settings **************************/
/************************************************************************/

/*
   I2C address
   Change this if you want to flash the firmware with a different I2C address.
   Alternatively, you can use setI2Caddress() from the library to change it later
*/
const uint8_t defaultAddress = 0x08; // default


/*!
  @brief Uncomment this to enable time keeping diagnostics. You probably should disable
  debugging, as Serial output will distort the measurements severely. Diagnostics
  take a little extra time and ressources, so you best disable it in production
  environments.
  @todo make diagnostics another module?
*/
//#define DIAGNOSTICS

/************************************************************************/
/******* end of firmware configuration settings **************************/
/************************************************************************/


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
   I2C stuff
*/

SimpleBuffer* bufferIn;
SimpleBuffer* bufferOut;
volatile uint8_t newMessage = 0; // signals main loop that a receiveEvent with valid new data occurred


/*
   EEPROM stuff
*/

#define EEPROM_OFFSET_I2C_ADDRESS 0 // where in eeprom is the I2C address stored, if any? [1 CRC8 + 4 marker + 1 address = 6 bytes]
const uint32_t eepromI2CaddressMarker = 0x12C0ACCF; // arbitrary 32bit marker proving that next byte in EEPROM is in fact an I2C address
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
const uint32_t eepromUsedSize = 6; // total bytes of EEPROM used by us
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)


/*!
  @brief Read a stored I2C address from EEPROM. If there is none, use default.
*/
uint8_t retrieveI2C_address()
{
#ifdef USE_EEPROM
  SimpleBuffer b;
  b.init(8);
  // read 6 bytes from eeprom: [0]=CRC8; [1-4]=marker; [5]=address
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.begin(256);
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("Reading from EEPROM: ");
  for (byte i = 0; i < 6; i++) {
    b.buffer[i] = EEPROM.read(EEPROM_OFFSET_I2C_ADDRESS + i);
    log(b.buffer[i]); log (" ");
  }
  log("\n");
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  EEPROM.end();
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("\n");
  // we wrote directly to the buffer, so reader idx is still at 1
  uint32_t markerTest; b.read(markerTest);
  uint8_t storedAddress; b.read(storedAddress); // now idx will be at 6, so that CRC check below will work
  if (b.checkCRC8() and (markerTest == eepromI2CaddressMarker)) {
    log("Stored address ");log(storedAddress);log("\n");
    return storedAddress;
  } else
#endif // USE_EEPROM
  {
    log("No stored address\n");
    return defaultAddress;
  }
}


/*!
  @brief Write I2C address to EEPROM
  @note ESP eeprom.h works a bit different from AVRs: https://arduino-esp8266.readthedocs.io/en/3.0.2/libraries.html#eeprom
*/
void storeI2C_address(uint8_t newAddress)
{
#ifdef USE_EEPROM
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
#elif defined(ARDUINO_ARCH_SAMD)
  log(" commit ");log(EEPROM.isValid());
  EEPROM.commit();
#endif // defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  log("\n");
#else // USE_EEPROM
  log("No EEPROM for this board architecture, storeI2C_address() did nothing.\n");
#endif // USE_EEPROM
}


/*
   Interrupt (to controller) stuff
   note: The interrupt pin is global, as there is only one shared by all modules
   and units. However, modules can implement them so that interrupts can be
   enabled for each unit seperately,
*/

int8_t interruptPin = -1; // -1 for undefined
bool interruptActiveHigh = true;
uint8_t interruptSource = 0xF;
uint8_t interruptReason = interruptReason_none;

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
  //asm volatile (" jmp 0");
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  ESP.restart();
#endif
}


/*
   Inject module declarations
*/

#define MF_STAGE MF_STAGE_declarations
#include "firmware_modules.h"
#undef MF_STAGE

void requestEvent(); // Forward declaration. Without it the Arduino magic will be confused by the compiler directives.

/**************************************************************************/
/*!
    @brief Setup system. Retrieve I2C address from EEPROM or default
    and initialize I2C target.
*/
/**************************************************************************/
void setup()
{

#if defined(DEBUG)
  Serial.begin(115200);

  // all chips with "native" usb require waiting till `Serial` is true
  unsigned int begin_time = millis();
  while (! Serial && millis() - begin_time < 1500) {
    delay(10);  // but at most 1 sec if not plugged in to usb
  }
#endif // DEBUG

  log("\n\n\n=== I2Cwrapper firmware v");
  log(I2Cw_VersionMajor); log("."); log(I2Cw_VersionMinor); log("."); log(I2Cw_VersionPatch); log(" ===\n");
  log("Running on architecture ");
#if defined(ARDUINO_ARCH_AVR)
  log("ARDUINO_ARCH_AVR\n");
#elif defined(ARDUINO_ARCbH_ESP8266)
  log("ARDUINO_ARCH_ESP8266\n");
#elif defined(ARDUINO_ARCH_ESP32)
  log("ARDUINO_ARCH_ESP32\n");
#elif defined(ARDUINO_ARCH_SAMD)
  log("ARDUINO_ARCH_SAMD\n");
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


  uint8_t i2c_address = retrieveI2C_address();
  setup_wire(i2c_address);

  bufferIn = new SimpleBuffer; bufferIn->init(I2CmaxBuf);
  bufferOut = new SimpleBuffer; bufferOut->init(I2CmaxBuf);

}

void setup_wire(uint8_t i2c_address) {
  Wire.begin(i2c_address);
  log("I2C started, listening to address "); log(i2c_address); log("\n\n");
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
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
    log("\n    > Cycles/s = "); log((cycles - lastCycles) * 1000 / reportPeriod); log("\n");
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
    log("\n\n");
    reportNow = false;
  }
#endif

  // Check for new incoming messages from I2C interrupt
  if (newMessage > 0) {
    processMessage(newMessage);
    newMessage = 0;
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



/**************************************************************************/
/*!
  @ brief Handle I2C receive event. Just read the message and inform main loop.
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

#if defined(DIAGNOSTICS)
  uint32_t thenMicrosP = micros();
#endif // DIAGNOSTICS

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
            log("\n\n---> Resetting\n\n");
            // Inject modules' reset code
#define MF_STAGE MF_STAGE_reset
#include "firmware_modules.h"
#undef MF_STAGE
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

            // most platforms support .end, so can do live update of listening address
            #ifdef WIRE_HAS_END
            Wire.end(); //
            setup_wire( newAddress );
            #endif
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
    writeOutputBuffer();
#endif  // ESP32

  } // if (bufferIn->checkCRC8())
  log("\n");


#if defined(DIAGNOSTICS)
  currentDiagnostics.lastProcessTime = micros() - thenMicrosP;
#endif // DIAGNOSTICS

}


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


/*!
  @brief Handle I2C request event. Will send results or information requested
    by the last command, as defined by the contents of the outputBuffer.
  @todo <del>Find sth. meaningful to report from requestEvent() if no message is
  pending, e.g. current position etc.</del> not implemented, ESP32 can't do that (doh)
*/

#if defined(ESP8266) || defined(ESP32)
void IRAM_ATTR requestEvent()
#else
void requestEvent()
#endif
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
