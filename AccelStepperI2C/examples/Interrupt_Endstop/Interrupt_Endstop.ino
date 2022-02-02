/*
  AccelStepperI2C interrupt & endstop demo
  (c) juh 2022

  1. install library to Arduino environment
  2. Upload firmware.ino to slave (Arduino Uno or ESP32).
  3. upload this example to master (Arduino Uno).
  4. connect the I2C bus of both devices (usually A4<>A4, A5<>A5, GND<>GND),
     don't fortget two I2C pullups and level-shifters if needed
  5. also connect +5V<>+5V to power one board from the other, if needed
  6. connect interrupt pin from slave (default: pin A3, CoolEN on GRBL CNC shields)
     to master (default: pin 2)
  7. connect one driver, one stepper, and two endstops to GND (sharing one input) and
     adapt pin definitions below if needed.
  8. now (not earlier) provide external power to the steppers and power to the Arduinos

*/

#include <Arduino.h>
#include <AccelStepperI2C.h>
#include <Wire.h>

//#define DEBUG_AccelStepperI2C

const uint8_t addr = 0x8; // i2c address of slave
const uint8_t stepPin = 2;
const uint8_t dirPin = 5;
const uint8_t endstopPin = 9;
const uint8_t interruptPinSlave = A3;
const uint8_t interruptPinMaster = 2; // Pins 2 and 3 are hardware interrupt pins on Arduino Uno
volatile bool interruptFlag = false; // volatile for interrupt

const float homingSpeed = 20.0; // in steps/second. Adapt this for your stepper/gear/microstepping setup
const float maxRunSpeed = 200.0; // in steps/second. Adapt this for your stepper/gear/microstepping setup

AccelStepperI2C* X; // do not invoke the constructors here, we need to establish I2C connection first.
long lowerEndStop, upperEndStop;

void setup() {

  Wire.begin(); // Important: initialize Wire before creating AccelStepperI2C objects
  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println("\n\n\nAccelStepperI2C demo - interrupts\n\n");

  // If the slave's and master's reset is not synchronized by hardware, after a master's reset the slave might
  // think the master wants another stepper, not a first one, and will run out of steppers, sooner or later.
  Serial.println("\n\nresetting slave\n");
  resetAccelStepperSlave(addr);
  delay(500);

  if (!checkVersion(addr)) {
    Serial.println("Warning: Master and slave are not using the same library version.");
    Serial.print("Master version is "); Serial.print(AccelStepperI2C_VersionMajor);
    Serial.print("."); Serial.println(AccelStepperI2C_VersionMinor);
    uint16_t v = getVersion(addr);
    Serial.print("Slave version is"); Serial.print(v >> 8);
    Serial.print("."); Serial.println(v & 0xFF);
    while (true) {}
  }

  // add and init stepper

  Serial.println("\nAdding stepper(s)");
  X = new AccelStepperI2C(addr,
                          AccelStepper::DRIVER, /* <-- driver type */
                          stepPin,              /* <-- step pin */
                          dirPin,               /* <-- dir pin */
                          0,                    /* <-- pin3 (unused for driver type DRIVER) */
                          0                     /* <-- pin4 (unused for driver type DRIVER) */
                         );
  if (X->myNum < 0 ) {
    while (1) {} // failed, halting.
  }

  // configure endstop

  X->setEndstopPin(endstopPin,   // <-- endstop pin
                   true,         // internal pullup
                   true          // activeLow works nicely together with internal pullup
                  );
  X->enableEndstops();

  // configure interrupt

  setInterruptPin(addr,
                  interruptPinSlave,  // <-- interrupt pin
                  true // activeHigh = master will have to look out for a RISING flank
                  );
  X->enableInterrupts(); // make slave send out interrupts for this stepper
  attachInterrupt(digitalPinToInterrupt(interruptPinMaster), interruptFromSlave, RISING); // make master notice them


  X->setMaxSpeed(maxRunSpeed);
  if (X->endstops() != 0) {
    Serial.println("\nWarning: Please move stepper away from endstop,\n"
                   "because I don't know in which direction I have to move to get away from it.\n\nHALTING.");
    while (true) {}
  }

  findEndstops();
  Serial.print("Endstops found at positions 0 (lower endstop) and ");
  Serial.print(upperEndStop); Serial.println(" (upper endstop).\n");

  Serial.println("\n\n\n\n");
  X->moveTo(upperEndStop / 2); // start with some initial target
  X->runState(); // go there with acceleration

}

void interruptFromSlave() {
  interruptFlag = true; // just set a flag, leave interrupt as quickly as possible
}

void waitForInterrupt() {
  while (!interruptFlag) {}
  interruptFlag = false;
}

void findEndstops() {

  X->setSpeed(-homingSpeed);            // start in negative direction, as we're looking for point zero first
  X->runSpeedState();                   // run at constant speed (= interrupt can only be triggered by endstop, not by target reached)
  waitForInterrupt();                   // and wait for endstop (state machine will stop there)
  X->setCurrentPosition(0);             // let's define this endstop as position 0
  lowerEndStop = 0;

  X->setSpeed(homingSpeed);             // now let's look in the positive direction
  X->runSpeedState();                   // run at constant (slow) speed
  waitForInterrupt();                   // and wait for endstop (state machine will stop there)
  upperEndStop = X->currentPosition();  // save it

}

/*
   For testing purposes, the loop will run the stepper to random targets
   which might be invalid, i.e. outside of the endstop limits. If it hits
   an endstop, it runs to the middle position, and starts over. If it
   arrives at a valid target posistion, it just sets another target.
*/
const long chance = 20; // % chance that target will be off limits
void loop() {

  waitForInterrupt();

  // determine what caused the interrupt, state machine will have stopped in any case
  if (X->endstops() != 0) {

    Serial.println("ran into endstop, returning to middle position.");
    X->moveTo(upperEndStop / 2);
    X->runState(); // go there with acceleration

  } else if (!X->isRunning()) {

    Serial.print("target reached, setting new target to ");
    long offLimits = (upperEndStop * chance / 100) / 2;
    long newPos = random(-offLimits, upperEndStop + offLimits) ;
    Serial.println(newPos);
    X->moveTo(newPos);
    X->runState(); // go there with acceleration

  } else {

    Serial.println("\nWarning: Interrupt due to unknown reason\n\nHALTING.");
    while (true) {}

  }

}
