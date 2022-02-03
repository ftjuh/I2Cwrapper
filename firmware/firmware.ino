/*!
   @file firmware.ino
   @brief Firmware for an I2C slave controlling up to 8 stepper motors via AccelStepper library
   @details This documentation is probably only relevant for you if you want to change the firmware.
   Otherwise, the library documentation should suffice.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @todo debugging
   @todo return messages (results) should ideally come with an id, too, so that master can be sure 
      it's the correct result. Currently only msg. length is checked.
   @todo volatile variables???
   @todo <del>make i2c address configurable with pins or via i2c/EEPROM</del>
   @todo <del>implement endstops/reference positions</del>
*/
#include <Arduino.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <AccelStepperI2C.h>
#include <SimpleBuffer.h>
#include <EEPROM.h>



/*!
 @brief Uncomment this to enable debugging output on Serial
*/
//#define DEBUG_AccelStepperI2C

/*!
 @brief Uncomment this to enable time keeping diagnostics. You probably should disable
 debugging, as Serial output will distort the measurements severely. Diagnostics 
 take a little extra time and ressources, so you best disable it in production 
 environments.
*/
#define DIAGNOSTICS_AccelStepperI2C


/*
   Debugging stuff
*/

#ifdef DEBUG_AccelStepperI2C
#define log(...)       Serial.print(__VA_ARGS__)
volatile uint8_t sentOnRequest = 0;
#else
#define log(...)
#endif // DEBUG_AccelStepperI2C



/*
   Diagnostics stuff
*/

#ifdef DIAGNOSTICS_AccelStepperI2C
bool diagnosticsEnabled = false;
uint32_t thenMicros;
diagnosticsReport currentDiagnostics;
uint32_t cycles = 0; // keeps count of main loop iterations
uint32_t previousLastReceiveTime; // used to delay receive times by one receive event
#endif // DIAGNOSTICS_AccelStepperI2C

#ifdef DEBUG_AccelStepperI2C
uint32_t now, then = millis();
bool reportNow = true;
#endif // DEBUG_AccelStepperI2C


/*
   EEPROM stuff
*/

#define EEPROM_OFFSET_I2C_ADDRESS 0 // where in eeprom is the I2C address stored, if any? [1 CRC8 + 4 marker + 1 address = 6 bytes]
uint32_t eepromI2CaddressMarker = 0x12C0ACCF; // arbitrary 32bit marker proving that next byte in EEPROM is in fact an I2C address


/*
   I2C stuff
*/

const uint8_t I2C_defaultAddress = 0x8;
//const uint8_t maxBuffer = 16; //
SimpleBuffer* bufferIn;
SimpleBuffer* bufferOut;
volatile uint8_t newMessage = 0; // signals main loop that a receiveEvent with valid new data occurred

/*!
  @brief Read a stored I2C address from EEPROM. If there is none, use default.
*/
uint8_t retrieveI2C_address() {
  SimpleBuffer b;
  b.init(8);
  // read 6 bytes from eeprom
  for (byte i = 0; i < 6; i++) {
    b.buffer[i] = EEPROM.read(EEPROM_OFFSET_I2C_ADDRESS + i);
  }
  b.idx = 6; // [0]=CRC8; [1-4]=marker; [5]=address
  uint32_t markerTest; b.read(markerTest);
  if (b.checkCRC8() and (markerTest == eepromI2CaddressMarker)) {
    uint8_t storedAddress; b.read(storedAddress);
    return storedAddress;
  } else {
    return I2C_defaultAddress;
  }
}

/*!
  @brief Write I2C address to EEPROM
*/
void storeI2C_address(uint8_t newAddress) {
  SimpleBuffer b;
  b.init(8);
  b.write(eepromI2CaddressMarker);
  b.write(newAddress);
  b.setCRC8();
  // write 6 bytes to eeprom
  //  eeprom_write_block((const void*)&b.buffer[0], (void*)EEPROM_OFFSET_I2C_ADDRESS, 6);
  for (byte i = 0; i < 6; i++) {
    EEPROM.write(EEPROM_OFFSET_I2C_ADDRESS + i, b.buffer[i]);
  }
}


/*
   Interrupt (to master) stuff
   note: The interrupt pin is global, as there is only one for all steppers together.
   However, interrupts can be enabled for each stepper seperately.
*/

int8_t interruptPin = -1;
bool interruptActiveHigh = true;

/*
   Endstop stuff
*/

struct Endstop {
  uint8_t pin;
  bool activeLow;
  //bool internalPullup; // we don't need to store this, we can directly use it when adding the pin
};
const uint8_t maxEndstops = 2; // not sure if there are scenarios where more than two make sense, but why not be prepared and make this a constant?


/*
   Stepper stuff
*/

const uint8_t maxSteppers = 8;
uint8_t numSteppers = 0; // number of initialised steppers
/*!
  @brief holds stepper parameters needed for local slave management
*/
struct Stepper
{
  AccelStepper * stepper;
  uint8_t state = state_stopped;
  Endstop endstops[maxEndstops];
  uint8_t numEndstops = 0;
  bool endstopsEnabled = false;
  bool interruptsEnabled = false;
};
Stepper steppers[maxSteppers];



// ugly but easy way to reset

void(* resetFunc) (void) = 0;//declare reset function at address 0



/**************************************************************************/
/*!
    @brief setup system
*/
/**************************************************************************/
void setup() {

#ifdef DEBUG_AccelStepperI2C
  Serial.begin(115200);
#endif
  log("\n\n\n=== AccelStepperI2C v0.1 ===\n\n");

  uint8_t i2c_address = retrieveI2C_address();
  Wire.begin(i2c_address);
  log("I2C started with address "); log(i2c_address); log("\n\n");
  while (Wire.available()) {
    uint8_t discard = Wire.read(); log(discard); log(" "); 
    discard++; // just to suppress compiler warning if logging is off...
  }
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  bufferIn = new SimpleBuffer; bufferOut = new SimpleBuffer;
  bufferIn->init(maxBuf);
  bufferOut->init(maxBuf);

}

/**************************************************************************/
/*!
    @brief Assign and initialize new stepper. Calls the
     <a href="https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#a3bc75bd6571b98a6177838ca81ac39ab">
      AccelStepper's[1/2] constructor</a>
    @returns number of stepper assigned to new stepper, -1 for error
    @note <a href="https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#afa3061ce813303a8f2fa206ee8d012bd">
    AccelStepper()[2/2] constructor</a> currently unsupported
*/
/**************************************************************************/
int8_t addStepper(uint8_t interface = AccelStepper::FULL4WIRE,
                  uint8_t pin1 = 2,
                  uint8_t pin2 = 3,
                  uint8_t pin3 = 4,
                  uint8_t pin4 = 5,
                  bool enable = true)
{
  if (numSteppers < maxSteppers)
  {
    steppers[numSteppers].stepper = new AccelStepper(interface, pin1, pin2, pin3, pin4, enable);
    steppers[numSteppers].state = state_stopped;
    log("Add stepper with internal myNum = "); log(numSteppers);
    return numSteppers++;
  }
  else
  {
    log("-- Too many steppers, failed to add new one\n");
    return -1;
  }
}


/*
   Read endstop(s) for stepper s.
   Returns bit pattern, last added endstop is LSB; 0 for inactive, 1 for active (taking activeLow in account)
   returns 0x0, if no endstops set
*/
uint8_t readEndstops(uint8_t s) {
  uint8_t res = 0;
  for (uint8_t i = 0; i < steppers[s].numEndstops; i++) {
    //log(i); log(" p");
    //log(digitalRead(steppers[s].endstops[i].pin));  log(" ");
    res = (res << 1) | (digitalRead(steppers[s].endstops[i].pin) ^ steppers[s].endstops[i].activeLow); // xor
  }
  //log(" res="); log(res);       log(" ");
  return res;
}


// interrupt master if interrupts enabled for source stepper
void triggerInterrupt(uint8_t source) {
  if (steppers[source].interruptsEnabled and interruptPin >= 0) {
    digitalWrite(interruptPin, interruptActiveHigh ? HIGH : LOW);
  }
}

void clearInterrupt() {
  if (interruptPin >= 0) {
    digitalWrite(interruptPin, interruptActiveHigh ? LOW : HIGH);
  }
}

/**************************************************************************/
/*!
    @brief Main loop, implements the state machine. Will check for each
    stepper's state and do the appropriate polling (run() etc.) as needed.
    Also checks for incoming commands and passes them on for processing.
*/
/**************************************************************************/

void loop()
{

#ifdef DEBUG_AccelStepperI2C
  now = millis();
  if (now > then + 1000) {
    reportNow = true;
    log("Cycles = "); log(cycles); log("  | Stepper states = ");
    then = now;
  }
#endif

  for (uint8_t i = 0; i < numSteppers; i++)    // cycle through all defined steppers
  {

#ifdef DEBUG_AccelStepperI2C
    if (reportNow) {
      log("  ["); log(i); log("]:"); log(steppers[i].state);
    }
#endif

    bool timeToCheckTheEndstops = false; // ###todo: change polling to pinchange interrupt
    switch (steppers[i].state)
    {

      case state_run: // boolean AccelStepper::run
        if (not steppers[i].stepper->run()) // target reached?
        {
          steppers[i].state = state_stopped;
          triggerInterrupt(i);
        }
        timeToCheckTheEndstops = true; // we cannot tell if there was a step, so we'll have to check every time. (one more reason to do it with interrupts)
        break;

      case state_runSpeed:  // boolean AccelStepper::runSpeed
        timeToCheckTheEndstops = steppers[i].stepper->runSpeed(); // true if stepped
        break;

      case state_runSpeedToPosition:  // boolean AccelStepper::runSpeedToPosition
        timeToCheckTheEndstops = steppers[i].stepper->runSpeedToPosition();  // true if stepped
        if (steppers[i].stepper->distanceToGo() == 0)
        {
          // target reached, stop polling
          steppers[i].state = state_stopped;
          triggerInterrupt(i);
        }
        break;

      case state_stopped: // do nothing
        break;
    } // switch

    if (timeToCheckTheEndstops and steppers[i].endstopsEnabled) {
      if (readEndstops(i) != 0) {
        steppers[i].state = state_stopped; // endstop reached, stop polling
        triggerInterrupt(i);
      }
    }

  } // for

  // Check for new incoming messages from interrupt.
  // ### Move into for-loop, so that it's checked more often?
  if (newMessage > 0) {
    processMessage(newMessage);
    newMessage = 0;
  }

#ifdef DEBUG_AccelStepperI2C
  if (sentOnRequest > 0) {
    log("Output buffer sent ("); log(sentOnRequest); log(" bytes): ");
    for (uint8_t j = 0; j < sentOnRequest; j++)
    {
      log(bufferOut->buffer[j], HEX);  log(" ");
    }
    log("\n");
    sentOnRequest = 0;
  }
#endif

#ifdef DEBUG_AccelStepperI2C
  if (reportNow) {
    log("\n");
    reportNow = false;
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
void receiveEvent(int howMany)
{

#ifdef DIAGNOSTICS_AccelStepperI2C
  thenMicros = micros();
#endif // DIAGNOSTICS_AccelStepperI2C

  if (newMessage == 0) { // Only accept message if an earlier one is fully processed.
    bufferIn->reset();
    for (uint8_t i = 0; i < howMany; i++) {
      bufferIn->buffer[i] = Wire.read();
      // uint8_t c = Wire.read();
      // bufferIn->write(c);
    }
    bufferIn->idx = howMany;
    newMessage = howMany; // tell main loop that new data has arrived

  } else { // discard message ### how can we warn the master??? Interrutp line would be good, here.
    while (Wire.available()) {
      Wire.read();
    }
  }

#ifdef DIAGNOSTICS_AccelStepperI2C
  // delay storing the executing time for one cycle, else diagnostics() would always return 
  // its own receive time, not the one of the previous command
  previousLastReceiveTime = micros() - thenMicros;
  currentDiagnostics.lastReceiveTime = previousLastReceiveTime;
#endif // DIAGNOSTICS_AccelStepperI2C

}


/**************************************************************************/
/*!
  @brief Process message received via I2C.
  Normal messages consist of at least three bytes:
  [0] CRC8 checksum
  [1] Command. Most commands correspond to an AccelStepper function, some
  are new for I2C reasons (see I2C_commands.h). Unknown commands are ignored.
  [2] Number of the stepper that is to receive the command. If no stepper with
     this number exists, the message is ignored, except for general commands like
     resetCmd, which ignore this parameter.
  [3 - maxBuffer] Optional parameter bytes. Each command that comes with too
   many or too few parameter bytes is ignored.
   @note Blocking functions runToPositionCmd() and runToNewPositionCmd() not
      implemented here. Blocking at the slave's side seems not too useful, as
      the master won't wait for us anyway,
    @todo Implement blocking functions runToPositionCmd() and runToNewPositionCmd()
    on the master's side/in the library?
*/
/**************************************************************************/
void processMessage(uint8_t len) {

#ifdef DIAGNOSTICS_AccelStepperI2C
  uint32_t thenMicrosP = micros();
#endif // DIAGNOSTICS_AccelStepperI2C


#ifdef DEBUG_AccelStepperI2C
  log("New message with "); log(len); log(" bytes. ");
  for (int j = 0; j < len; j++)  {
    log (bufferIn->buffer[j]); log(" ");
  }
#endif

  if (bufferIn->checkCRC8()) {  // ignore invalid message ### how to warn the master?

    bufferIn->reset(); // set reader to start
    uint8_t cmd; bufferIn->read(cmd);
    int8_t s; bufferIn->read(s); // stepper addressed (if any, will not be used for general commands > 200)
    log("CRC8 ok. Command = "); log(cmd); log(" for stepper "); log(s);
    log(" with "); log(len - 3); log(" parameter bytes --> ");

    if (((s >= 0) and (s < numSteppers)) or (cmd >= generalCommandsStart)) { // valid stepper *or* general command

      bufferOut->reset();  // let's hope last result was already requested by master, as now it's gone
      int8_t i = len - 3 ; // bytes received minus 3 header bytes = number of parameter bytes

      switch (cmd)
      {

        case moveToCmd: // void   moveTo (long absolute)
          {
            if (i == 4) // not nice to have these constants hardcoded here, but what the heck
            {
              // 1 long parameter
              long l = 0;
              bufferIn->read(l);
              steppers[s].stepper->moveTo(l);
            }
          }
          break;

        case moveCmd: // void   move (long relative)
          {
            if (i == 4)
            {
              // 1 long parameter
              long l = 0;
              bufferIn->read(l);
              steppers[s].stepper->move(l);
            }
          }
          break;

        // usually not to be called directly via I2C, use state machine instead
        case runCmd:  // boolean  run ()
          {
            if (i == 0) // no parameters
            {
              bool res = steppers[s].stepper->run();
              bufferOut->write(res);
            }
          }
          break;

        // usually not to be called directly via I2C, use state machine instead
        case runSpeedCmd: //  boolean   runSpeed ()
          {
            if (i == 0) // no parameters
            {
              bool res = steppers[s].stepper->runSpeed();
              bufferOut->write(res);
            }
          }
          break;

        case setMaxSpeedCmd:  // void   setMaxSpeed (float speed)
          {
            if (i == 4)
            {
              // 1 long parameter
              float f = 0;
              bufferIn->read(f);
              steppers[s].stepper->setMaxSpeed(f);
            }
          }
          break;

        case maxSpeedCmd: // float  maxSpeed ()
          {
            if (i == 0) // no parameters
            {
              float f = steppers[s].stepper->maxSpeed();
              bufferOut->write(f);
            }
          }
          break;

        case setAccelerationCmd:  // void   setAcceleration (float acceleration)
          {
            if (i == 4)
            {
              // 1 float parameter
              float f = 0;
              bufferIn->read(f);
              steppers[s].stepper->setAcceleration(f);
            }
          }
          break;

        case setSpeedCmd: // void   setSpeed (float speed)
          {
            if (i == 4)
            {
              // 1 float parameter
              float f = 0;
              bufferIn->read(f);
              steppers[s].stepper->setSpeed(f);
            }
          }
          break;

        case speedCmd:  // float  speed ()
          {
            if (i == 0) // no parameters
            {
              float f = steppers[s].stepper->speed();
              bufferOut->write(f);
            }
          }
          break;

        case distanceToGoCmd: // long   distanceToGo ()
          {
            if (i == 0) // no parameters
            {
              long l = steppers[s].stepper->distanceToGo();
              bufferOut->write(l);
            }
          }
          break;

        case targetPositionCmd: // long   targetPosition ()
          {
            if (i == 0) // no parameters
            {
              long l = steppers[s].stepper->targetPosition();
              bufferOut->write(l);
            }
          }
          break;

        case currentPositionCmd:  // long   currentPosition ()
          {
            if (i == 0) // no parameters
            {
              long l = steppers[s].stepper->currentPosition();
              bufferOut->write(l);
            }
          }
          break;

        case setCurrentPositionCmd: // void   setCurrentPosition (long position)
          {
            if (i == 4)
            {
              // 1 long parameter
              long l = 0;
              bufferIn->read(l);
              steppers[s].stepper->setCurrentPosition(l);
            }
          }
          break;

        // blocking, not implemented (yet?)
        case runToPositionCmd:  // void   runToPosition () {}
          break;

        // usually not to be called directly via I2C, use state machine instead
        case runSpeedToPositionCmd: // boolean  runSpeedToPosition ()
          {
            if (i == 0) // no parameters
            {
              bool res = steppers[s].stepper->runSpeedToPosition();
              bufferOut->write(res);
            }
          }
          break;

        // blocking, not implemented (yet?)
        case runToNewPositionCmd: // void   runToNewPosition (long position) {}
          break;

        case stopCmd: // void   stop ()
          {
            if (i == 0) // no parameters
            {
              steppers[s].stepper->stop();
            }
          }
          break;

        case disableOutputsCmd: // virtual void   disableOutputs ()
          {
            if (i == 0) // no parameters
            {
              steppers[s].stepper->disableOutputs();
            }
          }
          break;

        case enableOutputsCmd:  // virtual void   enableOutputs ()
          {
            if (i == 0) // no parameters
            {
              steppers[s].stepper->enableOutputs();
            }
          }
          break;

        case setMinPulseWidthCmd: // void   setMinPulseWidth (unsigned int minWidth)
          {
            if (i == 2)
            {
              // 1 uint16
              uint16_t minW = 0;
              bufferIn->read(minW);
              steppers[s].stepper->setMinPulseWidth(minW);
            }
          }
          break;

        case setEnablePinCmd: // void   setEnablePin (uint8_t enablePin=0xff)
          {
            if (i == 1)
            {
              // 1 uint8_t
              uint8_t pin = 0;
              bufferIn->read(pin);
              steppers[s].stepper->setEnablePin(pin);
            }
          }
          break;

        case setPinsInverted1Cmd: // void   setPinsInverted (bool directionInvert=false, bool stepInvert=false, bool enableInvert=false)
          {
            if (i == 1)
            {
              // 8 bits
              uint8_t b = 0;
              bufferIn->read(b);
              steppers[s].stepper->setPinsInverted(
                (b & 1 << 0) != 0, (b & 1 << 1) != 0, (b & 1 << 2) != 0);
            }
          }
          break;

        case setPinsInverted2Cmd: //  void  setPinsInverted (bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert)
          {
            if (i == 1)
            {
              // 8 bits
              uint8_t b;
              bufferIn->read(b);
              steppers[s].stepper->setPinsInverted(
                (b & 1 << 0) != 0, (b & 1 << 1) != 0, (b & 1 << 2) != 0, (b & 1 << 3) != 0, (b & 1 << 4) != 0);
            }
          }
          break;

        case isRunningCmd:  // bool   isRunning ()
          {
            if (i == 0) // no parameters
            {
              bool b = steppers[s].stepper->isRunning();
              bufferOut->write(b);
            }
          }
          break;


        // I2C wrapper commands


        case setStateCmd: //
          {
            if (i == 1) // 1 uint8_t
            {
              uint8_t newState;
              bufferIn->read(newState);
              steppers[s].state = newState;
            }
          }
          break;


        case getStateCmd: //
          {
            if (i == 0) // no parameters
            {
              bufferOut->write(steppers[s].state);
            }
          }
          break;


        case setEndstopPinCmd: //
          {
            if ((i == 3) and (steppers[s].numEndstops < maxEndstops))
            {
              int8_t pin; bufferIn->read(pin);
              bool activeLow; bufferIn->read(activeLow);
              bool internalPullup; bufferIn->read(internalPullup);
              steppers[s].endstops[steppers[s].numEndstops].pin = pin;
              steppers[s].endstops[steppers[s].numEndstops].activeLow = activeLow;
              pinMode(pin, internalPullup ? INPUT_PULLUP : INPUT);
              steppers[s].numEndstops++;
            }
          }
          break;

        case enableEndstopsCmd:
          {
            if (i == 1) // 1 bool
            {
              bool en; bufferIn->read(en);
              steppers[s].endstopsEnabled = en;
            }
          }
          break;

        case endstopsCmd:
          {
            if (i == 0) // no parameters
            {
              uint8_t b = readEndstops(s);
              bufferOut->write(b);
            }
          }
          break;

        case addStepperCmd: //
          {
            //log("addStepperCmd\n");
            if (i == 6)
            {
              // 5 uint8_t + 1 bool
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

        // @todo reset should be done smarter and completely in software w/o hard reset.
        case resetCmd:
          {
            if (i == 0) // no parameters
            {
              log("\n\n---> Resetting\n\n");
              for (uint8_t i = 0; i < numSteppers; i++) {
                steppers[i].stepper->stop();
                steppers[i].stepper->disableOutputs();
              }
            }
#ifdef DEBUG_AccelStepperI2C
            Serial.flush();
#endif
            resetFunc();
          }
          break;

        case changeI2CaddressCmd: //
          {
            // log("changeI2CaddressCmd\n");
            if (i == 1)  // 1 uint8_t
            {
              uint8_t newAddress; bufferIn->read(newAddress);
              storeI2C_address(newAddress);
            }
          }
          break;

        case enableDiagnosticsCmd: //
          {
            if (i == 1)  // 1 bool
            {
              bufferIn->read(diagnosticsEnabled);
            }
          }
          break;

        case diagnosticsCmd:
          {
            if (i == 0) // no parameters
            {
              currentDiagnostics.cycles = cycles;
              bufferOut->write(currentDiagnostics);
              cycles = 0;
            }
          }
          break;

        case setInterruptPinCmd: //
          {
            if (i == 2)
            {
              bufferIn->read(interruptPin);
              bufferIn->read(interruptActiveHigh);
              pinMode(interruptPin, OUTPUT);
              clearInterrupt();
            }
          }
          break;

        case enableInterruptsCmd: //
          {
            if (i == 1)  // 1 bool
            {
              bufferIn->read(steppers[i].interruptsEnabled);
            }
          }
          break;

        case getVersionCmd:
          {
            if (i == 0) // no parameters
            {
              bufferOut->write(AccelStepperI2C_VersionMajor);
              bufferOut->write(AccelStepperI2C_VersionMinor);
            }
          }
          break;


        default:
          log("No matching command found");

      } // switch

#ifdef DEBUG_AccelStepperI2C
      log("Output buffer after processing ("); log(bufferOut->idx); log(" bytes): ");
      for (uint8_t i = 0; i < bufferOut->idx; i++)
      {
        log(bufferOut->buffer[i], HEX);  log(" ");
      }
      log("\n");
#endif

#ifdef ESP32
      // ESP32 slave implementation needs the I2C-reply buffer prefilled. I.e. onRequest() will, without our own doing,
      // just send what's in the I2C buffer at the time of the request, and anything that's written during the request
      // will only be sent in the next request cycle. So let's prefill the buffer here.
      // ### what exactly is the role of slaveWrite() vs. Write(), here?
      // ### slaveWrite() is only for ESP32, not for it's poorer cousins ESP32-S2 and ESP32-C3. Need to fine tune the compiler directive, here?
      bufferOut->setCRC8();
      Wire.slaveWrite(bufferOut->buffer, bufferOut->idx);
#ifdef DEBUG_AccelStepperI2C
      log("sent "); log(bufferOut->idx); log(" bytes: "); // ### good boys don't use Serial in interrupts, it's said they don't work here
      for (uint8_t i = 0; i < bufferOut->idx; i++)
      {
        log(bufferOut->buffer[i], HEX);  log(" ");
      }
      log("\n");
#endif // DEBUG
#endif  // ESP32

    } // if valid s

  } // if (bufferIn->checkCRC8())
  log("\n");


#ifdef DIAGNOSTICS_AccelStepperI2C
  currentDiagnostics.lastProcessTime = micros() - thenMicrosP;
#endif // DIAGNOSTICS_AccelStepperI2C

}

/**************************************************************************/
/*!
  @brief Handle I2C request event. Will send results or information requested
    by the last command, as defined by the contents of the outputBuffer.
  @todo Find sth. meaningful to report from requestEvent() if no message is
  pending, e.g. current position etc.
*/
/**************************************************************************/
void requestEvent()
{

#ifdef DIAGNOSTICS_AccelStepperI2C
  thenMicros = micros();
#endif // DIAGNOSTICS_AccelStepperI2C

  if (bufferOut->idx > 1)
  {

#ifndef ESP32
    // ESP32 I2C buffer has already been prefilled in processMessage(), so nothing to do here for ESP32's
    bufferOut->setCRC8();
    Wire.write(bufferOut->buffer, bufferOut->idx);

#ifdef DEBUG_AccelStepperI2C
    log("sent "); log(bufferOut->idx); log(" bytes: "); // ### good boys don't use Serial in interrupts
    for (uint8_t i = 0; i < bufferOut->idx; i++)
    {
      log(bufferOut->buffer[i], HEX);  log(" ");
    }
    log("\n");
#endif // DEBUG

#endif // not ESP32

#ifdef DEBUG_AccelStepperI2C
    sentOnRequest = bufferOut->idx; // signal main loop that we sent buffer
#endif

    bufferOut->reset();  // never send anything twice

  } else {
    // ### find sth. meaningful to report if no message is pending
    // ### however it seems this won't work for ESP32 as they need the buffer prefilled before the requestEvent
  }

#ifdef DIAGNOSTICS_AccelStepperI2C
  currentDiagnostics.lastRequestTime = micros() - thenMicros;
#endif // DIAGNOSTICS_AccelStepperI2C

}
