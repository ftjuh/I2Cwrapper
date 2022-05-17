/*
  I2Cwrapper errror checking demo
  (c) juh 2022

  A variant of the AccelStepperI2C sweep demo that demonstrates the available error checking
  and safeguarding capabilities.

  If and how often you use them in your own sketches will depend on your
  degree of paranoia and the severity of potential harm done by uncontrolled
  hardware driven by the target device.

  Note that checking for errors is done at the wrapper's level, as it is
  the wrapper which handles communication for the stepper object.

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

long target = 3000; // initial target for sweep motion


I2Cwrapper wrapper(addr); // each target device is represented by a wrapper...
AccelStepperI2C stepper(&wrapper); // ...that the stepper uses to communicate with the target


void setup()
{

  Serial.begin(115200);
  Wire.begin();
  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side

  Serial.println("\n\n\n*** I2Cwrapper errror checking demo ***\n\n");

  /*
     Use this to see if the target device is listening.
  */

  if (!wrapper.ping()) {
    halt("Target not found! Check connections and restart.");
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }

  wrapper.reset(); // reset the target device
  delay(500); // and give it time to reboot

  printVersions();

  /*
     Use this to see if controller and target use the same version of the library
  */

  if (!wrapper.checkVersion(I2Cw_Version)) {
    /*
       Note: according to semver versioning (https://semver.org/), API changes
       are indicated by upping the major version. So in real world settings is
       should be sufficient to check if the major version numbers match,
       granted that future version of this software will implement the semver
       version as intended.
    */
    halt("Warning: Controller and target are not using the same library version.");
  } else {
    Serial.println("Target's and controller's versions match. Proceeding\n");
  }


  /*
      Attaching will usually only fail if the target runs out of its (default) 
      eight preallocated drivers (that's why it's important to reset the target 
      together with the controller, see above).
  */

  stepper.attach(AccelStepper::DRIVER, stepPin, dirPin);
  if (stepper.myNum < 0) { // should not happen after a reset
    halt("Error: stepper could not be allocated.");
  }

  /*
     Let's set up the system. Instead of checking each single transmission, we'll use
     transmissionErrors() at the end, as nothing critical is happening until then.
  */

  stepper.setEnablePin(enablePin);
  stepper.setPinsInverted(false, false, true);  // directionInvert, stepInvert, enableInvert
  stepper.enableOutputs();
  stepper.setMaxSpeed(maxRunSpeed);
  stepper.setAcceleration(acceleration);

  if (wrapper.transmissionErrors() > 0) { // check for an error in any of the above transmissions
    halt("Error: at least one transmission during setup failed.");
  } else {
    Serial.println("Target device successfully configured. Proceeding.");
  }

}


void loop()
{

  stepper.moveTo(target);
  if (!wrapper.sentOK ) {
    halt("Couldn't set target.\n");
  }

  stepper.runState(); // (re)start the state machine
  if (!wrapper.sentOK ) {
    halt("Couldn't activate state machine.\n");
  }

  // isRunning() is non void, i.e. it comprises a command sent and a result received,
  // so we'll need to check for both of them (or check for transmissionErrors()==0)
  while (stepper.isRunning() and wrapper.sentOK and wrapper.resultOK) {
    delay(200);
  };

  if (wrapper.transmissionErrors() > 0) {
    halt("Error while waiting for state machine to stop.");
  }

  Serial.println("<---> Target reached, turning around");
  target = - target;

}


void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}


void printVersions() {

  Serial.print("Controller's version of I2Cwrapper package is ");
  Serial.print(I2Cw_VersionMajor); Serial.print(".");
  Serial.print(I2Cw_VersionMinor); Serial.print(".");
  Serial.println(I2Cw_VersionPatch);

  uint32_t v = wrapper.getVersion();
  Serial.print("Target's version of I2Cwrapper package is ");
  Serial.print(v >> 16 & 0xff); Serial.print(".");
  Serial.print(v >> 8 & 0xff); Serial.print(".");
  Serial.println(v & 0xff);

}
