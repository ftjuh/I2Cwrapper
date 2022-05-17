/*
  AccelStepperI2C interrupt & endstop demo
  (c) juh 2022

  Uses two end stops and interrupts to home a stepper, and then run
  two tests: First, move the stepper to random positions between,
  and occasionaly beyond, the two endstops. Then, repeatedley homes at
  different speeds and reports reproducability of end stop positions.

  As it uses interrupts to detect target-reached or endstop-hit
  conditions, it does not need to poll the target at all.

  Assumes that controller and target both run on an Arduino Uno/Nano.
  See config section below for hardware setup.

  Warning: This sketch intentionally moves beyond the endstops, so set up
  your test scenario according to the safety precautions in README.md.

*/

#include <Arduino.h>
#include <AccelStepperI2C.h>
#include <Wire.h>


// --- Config. Change as needed for your hardware setup ---

const uint8_t addr = 0x8;                     // i2c address of target
const uint8_t stepPin = 8;                    // stepstick driver pin
const uint8_t dirPin = 7;                     // stepstick driver pin
const uint8_t enablePin = 2;                  // stepstick driver pin
const uint8_t endstopPin = 17;                // Arduino Uno/Nano pin A3 (it's safer to use raw pin nr., since "A3" might mean sth. different if the controller uses a different hardware platform)
const uint8_t interruptPinTarget = 9;          // needs to be wired to interruptPinController
const uint8_t interruptPinController = 2;         // wired to interruptPinTarget, needs to be a hardware interrupt pin (2 or 3 on Arduino Uno/Nano)
const float homingSpeed  = 100.0;             // in steps/second. Adapt this for your stepper/gear/microstepping setup
const float maxRunSpeed  = 600.0;             // in steps/second. Adapt this for your stepper/gear/microstepping setup
const float acceleration = maxRunSpeed / 4;   // 4 seconds from 0 to max speed. Adapt this for your stepper/gear/microstepping setup

// --- End of config ---


I2Cwrapper wrapper(addr); // each target device is represented by a wrapper...
AccelStepperI2C stepper(&wrapper); // ...that the stepper uses to communicate with the target

volatile bool interruptFlag = false; // volatile for interrupt
long lowerEndStopPos, upperEndStopPos, middlePos, range;
long lower, upper;



/***********************************************************************************

   ISR

 ************************************************************************************/
// this has to come before setup(), else the Arduino environment will be confused by the conditionals and macros
#if defined(ESP8266) || defined(ESP32)
void IRAM_ATTR interruptFromTarget()
#else
void interruptFromTarget()
#endif
{
  interruptFlag = true; // just set a flag, leave interrupt as quickly as possible
}

/***********************************************************************************

   SETUP

 ************************************************************************************/
void setup()
{

  Serial.begin(115200);
  Wire.begin();
  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side


  /*
     Prepare target device
  */

  if (!wrapper.ping()) {
    Serial.println("Target not found! Check connections and restart.");
    while (true) {}
  }
  wrapper.reset(); // reset the target device
  delay(500); // and give it time to reboot


  /*
     Setup stepper/stepstick driver
  */

  stepper.attach(AccelStepper::DRIVER, stepPin, dirPin);
  if (stepper.myNum < 0) { // should not happen after a reset
    Serial.println("Error: stepper could not be allocated");
    while (true) {}
  }
  stepper.setEnablePin(enablePin);
  stepper.setPinsInverted(false, false, true);  // directionInvert, stepInvert, enableInvert
  stepper.enableOutputs();
  stepper.setMaxSpeed(maxRunSpeed);
  stepper.setAcceleration(acceleration);


  /*
     Setup endstops. Both are connected to the same pin, so we need to set the pin only once.
  */

  stepper.setEndstopPin(endstopPin, /* activeLow */ true, /* internal pullup */ true); // activeLow since endstop switches pull the endstop pin to GND
  stepper.enableEndstops(); // make the state machine check for the endstops
  if (stepper.endstops() != 0) { // check if one of the endstops is currently activated
    Serial.println("\nWarning: Please move stepper away from endstop,\n"
                   "because I don't know in which direction I have to move to get away from it.\n\nHALTING.");
    stepper.disableOutputs(); // cut power, so that stepper can be moved away manually
    while (true) {}
  }


  /*
     Configure interrupts
  */

  // First, make the target send interrupts. The interrupt pin is shared by all units (steppers)
  // which use interrupts, so we need the wrapper to set it up.
  wrapper.setInterruptPin(interruptPinTarget, /* activeHigh */ true); // activeHigh -> controller will have to look out for a RISING flank
  stepper.enableInterrupts(); // make target send out interrupts for this stepper at the pin set above

  // And now make the controller listen for the interrupt
  pinMode(interruptPinController, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPinController), interruptFromTarget, RISING);


  /*
     All set up and ready to go.
  */

  findEndstops(homingSpeed);  // find upperEndStopPos and lowerEndStopPos;
  range = upperEndStopPos - lowerEndStopPos;
  middlePos = lowerEndStopPos + range / 2;

  stepper.moveTo(middlePos); // start in the middle
  stepper.runState(); // go there with acceleration

  // First test:
  // do some random movements, causing endstop and target-reached interrupt conditions
  randomWalk(25, 50); // 25 repetitions, 50% offshoot chance

  stepper.moveTo(middlePos); // go back to the middle before entering loop()
  stepper.runState();
  waitForInterrupt();

  // save endstop positions
  lower = lowerEndStopPos;
  upper = upperEndStopPos;

}


/***********************************************************************************

   LOOP: Second test, test reproducability of endstop positions

 ************************************************************************************/
long cycles = 0;
void loop()
{

  float sp = random(homingSpeed * 0.5, homingSpeed * 1.5); // vary speed a little
  findEndstops(sp);
  Serial.print("Deviations after "); Serial.print(cycles);
  Serial.print(" cycles, measured with a speed of "); Serial.print(sp);
  Serial.print(" steps/s: lower endstop = "); Serial.print(lowerEndStopPos - lower);
  Serial.print(", upper endstop = "); Serial.println(upperEndStopPos - upper);
  cycles++;

}


/***********************************************************************************

   REST OF THE SCHÜTZENFEST

 ************************************************************************************/

void findEndstops(float sp)
{

  stepper.setSpeed(-sp);  // start in negative direction, as we're looking for point zero first
  stepper.runSpeedState();  // run at constant speed (= interrupt can only be triggered by endstop, not by target reached)
  if (waitForInterrupt() != interruptReason_endstopHit) { // wait for endstop and check for correct interrupt cause
    Serial.print("Error. Interrupt due to unexpected reason #");
    while (true) {}
  }

  lowerEndStopPos = stepper.currentPosition();  // save position (usually, it would make sense to simply set this to 0, but we don't for testing purposes)

  // note: we're still at the endstop. But the interrupt will only trigger on the next not-bouncing induced activated-flank, so no problem here
  stepper.setSpeed(sp);             // now let's look in the positive direction
  stepper.runSpeedState();                   // run at constant (slow) speed
  if (waitForInterrupt() != interruptReason_endstopHit) { // and wait for other endstop
    Serial.print("Error. Interrupt due to unexpected reason.");
    while (true) {}
  }
  upperEndStopPos = stepper.currentPosition();  // save position

  Serial.print("Endstops found at positions "); Serial.print(lowerEndStopPos);
  Serial.print(" (lower endstop) and "); Serial.print(upperEndStopPos);
  Serial.println(" (upper endstop).\n\n");

}



/*
   wait and return reason for interrupt
*/
uint8_t waitForInterrupt()
{
  while (!interruptFlag) {} // wait until interrupt will set the flag ### will probably need a yield() on ESPs
  interruptFlag = false; // clear flag
  uint8_t reasonAndUnit = wrapper.clearInterrupt();
  if ((reasonAndUnit & 0xf) != stepper.myNum) { // stepper is the only unit in use, so there should be no other unit causing an interrupt
    Serial.println("Error. Interrupt cause from unknown unit.");
    while (true) {}
  }
  return reasonAndUnit >> 4; // dump unit and return reason in LSBs
}



/*
   This test routine will run the stepper repeatedly to random targets, some of
   them intentionally invalid, i.e. outside of the endstop limits. If it hits
   an endstop, it runs to the middle position, and starts over. If it arrives
   at a valid target posistion, it just sets another random target.
*/
void randomWalk(int repetitions, long chance) // % chance that target will be off limits
{

  long offLimits = (range * chance / 100) / 2;

  for (int j = 0; j < repetitions; j++) {

    uint8_t reason = waitForInterrupt();

    switch (reason) {
      case interruptReason_targetReachedByRun: {
          Serial.print("Target reached as planned, setting new target to ");
          long newPos = random(lowerEndStopPos - offLimits, upperEndStopPos + offLimits);
          Serial.println(newPos);
          stepper.moveTo(newPos);
          stepper.runState(); // go there with acceleration
          break;
        }
      case interruptReason_endstopHit: {
          Serial.println("Ran into endstop, returning to middle position.");
          stepper.moveTo(middlePos);
          stepper.runState(); // go there with acceleration
          break;
        }
      default: {
          Serial.print("Unexpected interrupt reason #");
          Serial.println(reason);
          while (true) {}
        }
    } // switch
  }
} // void randomWalk
