/*!
 *  @file AccelStepperI2C_firmware.h
 *  @brief Firmware module for the I2Cwrapper firmware.
 *  
 *  Provides control of up to eight stepper motors with up to two endstops each
 *  connected to the I2C target.
 * 
 *  @section author Author
 *  Copyright (c) 2022 juh
 *  @section license License
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2.
 */

/*
 * Note: Code injection will confuse doxygen, as withouth the including 
 * firmware.ino it looks like incomplete code. So, to prevent it from generating 
 * all kinds of gibberish documentation, the rest of this file is enclosed in 
 * @cond/@endcond tags to make doxygen ignore it. Usually, there's nothing of
 * interest to document for end users anyway.
 */

/// @cond
 
/*##########################################################################################*/
/*# MF_STAGE_includes ######################################################################*/
/*##########################################################################################*/

#if MF_STAGE == MF_STAGE_includes
#include <AccelStepperI2C.h>
#endif // MF_STAGE_includes



/*##########################################################################################*/
/*# MF_STAGE_declarations ##################################################################*/
/*##########################################################################################*/

#if MF_STAGE == MF_STAGE_declarations

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

/*
  This struct comprises all stepper parameters needed for local target management
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
  Assign and initialize new stepper. Calls the
    <a href="https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#a3bc75bd6571b98a6177838ca81ac39ab">
    AccelStepper's[1/2] constructor</a>.
  Returns internal number (0-7) of stepper assigned to new stepper, -1 for error
*/
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
    res = (res << 1) | (digitalRead(steppers[s].endstops[i].pin) ^ steppers[s].endstops[i].activeLow); // xor
  }
  return res;
}


/*
   Interrupt controller if interrupts are enabled for the source stepper.
*/
void triggerStepperInterrupt(uint8_t source, uint8_t reason)
{
  if (steppers[source].interruptsEnabled) {
    triggerInterrupt(source, reason);
  }
}

bool validStepper(int8_t s)
{
  return (s >= 0) and (s < numSteppers);
}

#endif // MF_STAGE_declarations



/*##########################################################################################*/
/*# MF_STAGE_setup #########################################################################*/
/*##########################################################################################*/



#if MF_STAGE == MF_STAGE_setup
log("AccelStepperI2C module enabled.\n");
#endif // MF_STAGE_setup




/*##########################################################################################*/
/*# MF_STAGE_loop ##########################################################################*/
/*##########################################################################################*/

/*
 * Implements the state machine. Will check for each stepper's state and do the 
 * appropriate polling (run() etc.) as needed.
 * @todo endstop polling: overhead to check if polling is needed
 * (timeToCheckTheEndstops) might cost more than it saves, so maybe just check
 * each cycle even if it might be much more often than needed.
 * 
 */

#if MF_STAGE == MF_STAGE_loop

#if defined(DEBUG)
if (reportNow and numSteppers > 0) { //  report state machine states
  log("  [Steppers]:states =");
}
#endif // defined(DEBUG)


for (uint8_t i = 0; i < numSteppers; i++) {  // cycle through all defined steppers

#if defined(DEBUG)
  if (reportNow) { // report state machine states for this stepper
    log("  ["); log(i); log("]:"); log(steppers[i].state);
  }
#endif // defined(DEBUG)

  bool timeToCheckTheEndstops = false; // ###todo: change polling to pinchange interrupt
  // ### do we need this at all? Why not just poll each cycle? It doesn't take very long.
  switch (steppers[i].state) {

    case state_run: // boolean AccelStepper::run
      if (not steppers[i].stepper->run()) { // target reached?
        steppers[i].state = state_stopped;
        triggerStepperInterrupt(i, interruptReason_targetReachedByRun);
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
        triggerStepperInterrupt(i, interruptReason_targetReachedByRunSpeedToPosition);
      }
      break;

    case state_stopped: // do nothing
      break;
  } // switch

  if (timeToCheckTheEndstops and steppers[i].endstopsEnabled) { // the stepper (potentially) stepped a step, so let's look at the endstops
    uint8_t es = pollEndstops(i);
    if (es != steppers[i].prevEndstopState) { // detect rising *or* falling flank
      uint32_t ms = millis();
      if (ms > steppers[i].endstopDebounceEnd) { // primitive debounce: ignore endstops for some ms after each new flank
        // log("** es: flank detected  \n");
        steppers[i].endstopDebounceEnd = ms + endstopDebouncePeriod; // set end of debounce period
        steppers[i].prevEndstopState = es;
        if (es != 0) { // this is a non-bounce, *rising* flank: stop stepper and trigger interrupt
          log("   Endstop detected!\n");
          //steppers[i].stepper->stop();
          steppers[i].stepper->setSpeed(0);
          steppers[i].stepper->moveTo(steppers[i].stepper->currentPosition());
          steppers[i].state = state_stopped; // endstop reached, stop polling
          triggerStepperInterrupt(i, interruptReason_endstopHit);
        }
      }
    }
  } // check endstops

} // for
#endif // MF_STAGE_loop



/*##########################################################################################*/
/*# MF_STAGE_processMessage ################################################################*/
/*##########################################################################################*/



#if MF_STAGE == MF_STAGE_processMessage

/*
   AccelStepper commands
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

// blocking, implemented in controller library
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

// blocking, implemented in controller library
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


#endif // MF_STAGE_processMessage



/*##########################################################################################*/
/*# MF_STAGE_reset #########################################################################*/
/*##########################################################################################*/


#if MF_STAGE == MF_STAGE_reset
for (uint8_t j = 0; j < numSteppers; j++) {
  steppers[j].stepper->stop();
  steppers[j].stepper->disableOutputs();
  for (uint8_t k = 0; k < steppers[j].numEndstops; k++) {   // reset endstops
    pinMode(steppers[j].endstops[k].pin, INPUT); // INPUT is Arduino default
  }
  delete steppers[j].stepper; // destroy object allocated earlier with new(). Note: will throw a compiler warning, as AccelStepper has no virtual destructor. This is without consequence, as we're not using the class polymorphically.
}
numSteppers = 0;
#endif // MF_STAGE_reset


/// @endcond
