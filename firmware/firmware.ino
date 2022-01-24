/*!
   @file AccelStepperI2Cfirmware.ino
   @brief Firmware for an I2C slave controlling up to 8 stepper motors via AccelStepper library
   @note serialization of numbers is machine dependent and only tested
    for ATmega Arduinos ATM, might break with different devices due to different
      endiands and bytes used for different data types.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @todo make i2c address configurable with pins or via i2c/EEPROM
   @todo implement endstops/reference positions
   @todo volatile variables???
   @todo debugging
   @todo return messages (results) should come with an id, too, so that master can be sure it's the correct result. Currently only msg. length is checked.
*/
#include <Arduino.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <AccelStepperI2C.h>
#include <SimpleBuffer.h>


#define DEBUG_AccelStepperI2C

#ifdef DEBUG_AccelStepperI2C
#define log(...)       Serial.print(__VA_ARGS__)
#else
#define log(...)
#endif


// I2C stuff

const uint8_t I2C_address = 0x8;  // ### make configurable
const uint8_t maxBuffer = 16; //
SimpleBuffer* bufferIn;
SimpleBuffer* bufferOut;
volatile uint8_t newMessage = 0; // signals main loop that a receiveEvent with valid new data occurred


// endstop stuff

struct Endstop {
  uint8_t pin;
  bool activeLow;
  //bool internalPullup; // we don't need to store this, we can directly use it when adding the pin
};
const uint8_t maxEndstops = 2; // not sure if there are scenarios where more than two make sense, but why not be prepared?



// stepper stuff

const uint8_t maxSteppers = 8;
volatile uint8_t numSteppers = 0; // number of initialised steppers
struct Stepper  // holds stepper parameters needed for local slave management
{
  AccelStepper * stepper;
  uint8_t state = state_stopped;
  // bool runResult; // holds most recent result of calls to runSpeed() or runSpeedToPosition()
  Endstop endstops[maxEndstops];
  uint8_t numEndstops = 0;
  bool endstopsEnabled = false;
};
volatile Stepper steppers[maxSteppers];


// ugly but easy way to reset

void(* resetFunc) (void) = 0;//declare reset function at address 0



/**************************************************************************/
/*!
    @brief setup system
*/
/**************************************************************************/
void setup()
{
#ifdef DEBUG_AccelStepperI2C
  Serial.begin(115200);
#endif
  Wire.begin(I2C_address);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  bufferIn = new SimpleBuffer; bufferOut = new SimpleBuffer;
  bufferIn->init(maxBuffer);
  bufferOut->init(maxBuffer);
  log("\n\n\n=== AccelStepperI2C v0.1 ===\n\n");
}

/**************************************************************************/
/*!
    @brief Assign and initialize new stepper. Calls the
     <a href="https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#a3bc75bd6571b98a6177838ca81ac39ab">
      AccelStepper's[1/2] constructor</a>
    @returns number of stepper assigned to new stepper, -1 for error
    @note < a href="https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#afa3061ce813303a8f2fa206ee8d012bd">
    AccelStepper()[2/2] constructor</a > currently unsupported
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
    log("Add stepper with internal myNum = ");
    log(numSteppers);
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

// void sendInterrupt(uint8_t cause) {
// }

/**************************************************************************/
/*!
    @brief Main loop, implements the state machine (everything else is done
    in the I2C-interrupts). Will check for each stepper's state and do the
    appropriate polling (run() etc.) as needed.
*/
/**************************************************************************/

#ifdef DEBUG_AccelStepperI2C
uint32_t now, then = millis();
uint32_t cycles = 0;
bool reportNow = true;
#endif

void loop()
{

#ifdef DEBUG_AccelStepperI2C
  now = millis();
  if (now > then + 2000) {
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

    bool timeToCheckTheEndstops = false; // @todo: change polling to pinchange interrupt
    switch (steppers[i].state)
    {

      case state_run: // boolean AccelStepper::run
        if (not steppers[i].stepper->run()) // target reached?
        {
          steppers[i].state = state_stopped;
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
        }
        break;

      case state_stopped: // do nothing
        break;
    } // switch

    if (timeToCheckTheEndstops and steppers[i].endstopsEnabled) {
      if (readEndstops(i) != 0) {
        steppers[i].state = state_stopped; // endstop reached, stop polling
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
  if (reportNow) {
    log("\n");
    cycles = 0; reportNow = false;
  }
  cycles++;
#endif
}

/**************************************************************************/
/*!
  @brief Handle I2C receive event. Just read the message and inform main loop.
  @todo <del>Just read the buffer here, move interpretation out of the interrupt.</del>
*/
/**************************************************************************/
void receiveEvent(int howMany)
{
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
}


/**************************************************************************/
/*!
  @brief Process message receive via I2C.
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
  log("New message with "); log(len); log(" bytes. ");
#ifdef DEBUG_AccelStepperI2C
  for (int j = 0; j < len; j++)  {
    log (bufferIn->buffer[j]); log(" ");
  }
#endif

  if (bufferIn->checkCRC8()) {  // ignore invalid message ### how to warn the master?

    bufferIn->reset(); // set reader to start
    uint8_t cmd; bufferIn->read(cmd);
    int8_t s; bufferIn->read(s); // stepper adressed (if any, will not be used for general commands > 200)
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

        // @todo reset should be done smarter and completely in software w/o hard reset.
        case resetCmd:
          {
            log("\n\n---------------> Resetting\n\n");
            for (uint8_t i = 0; i < numSteppers; i++) {
              steppers[i].stepper->stop();
              steppers[i].stepper->disableOutputs();
            }
#ifdef DEBUG_AccelStepperI2C
            Serial.flush();
#endif
            resetFunc();
          }
          break;

        case addStepperCmd: //
          {
            log("addStepperCmd: ");
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

        default:
          log("No matching command found");

      } // switch
    } // if valid s

  } // if (bufferIn->checkCRC8())
  log("\n");
}

/**************************************************************************/
/*!
  @brief Handle I2C request event. Will send results or information requested
    by the last command, as defined by the contents of the outputBuffer.
  @todo Find sth. meaningful to report from requestEvent() if no message is 
  pending, e.g. current position etc.
/*
/**************************************************************************/
void requestEvent()
{
  if (bufferOut->idx > 1)
  {
    bufferOut->setCRC8();
    log("sending "); log(bufferOut->idx); log(" bytes: "); // ### good boys don't use Serial in interrupts, supposedly they don't work here
    Wire.write(bufferOut->buffer, bufferOut->idx);
    for (uint8_t i = 0; i < bufferOut->idx; i++)
    {
      log(bufferOut->buffer[i], HEX);  log(" ");
    }
    log("\n");
    bufferOut->reset();  // never send anything twice
  } else { /// ### find sth. meaningful to report if no message is pending
    //    uint8_t res = 0;
    //    if (numSteppers > 0)
    //    {
    //      for (uint8_t i = 0; i < numSteppers; i++)
    //      {
    //        // send some diagnostics
    //      }
    //    }
    //    Wire.write(res);
  }
}
