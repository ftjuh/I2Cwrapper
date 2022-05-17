/*
   AccelStepperI2C Bounce demo
   (c) juh 2022

   This is a 1:1 equivalent of the AccelStepper Bounce.pde example
   https://www.airspayce.com/mikem/arduino/AccelStepper/Bounce_8pde-example.html
   
   Needs AccelStepperI2C.h module enabled in the target's firmware_modules.h.

*/

#include <Wire.h>
#include <AccelStepperI2C.h>


uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress); // each target device is represented by a wrapper...
AccelStepperI2C stepper(&wrapper); // ...that the stepper uses to communicate with the target

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side

  if (!wrapper.ping()) {
    Serial.println("Target not found! Check connections and restart.");
    while (true) {}
  }

  wrapper.reset(); // reset the target device
  delay(500); // and give it time to reboot

  stepper.attach(); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
  // attach() replaces the AccelStepper constructor, so it could also be called like this: 
  // stepper.attach(AccelStepper::DRIVER, 5, 6);
  
  if (stepper.myNum < 0) { // should not happen after a reset
    Serial.println("Error: stepper could not be allocated");
    while (true) {}
  }

  // Change these to suit your stepper if you want
  stepper.setMaxSpeed(2000);
  stepper.setAcceleration(50);
  stepper.moveTo(30000);

}

/* This is the recommended AccelStepperI2C implementation using the state machine.
 * Note that the polling frequency is not critical, as the state machine will stop
 * on its own. So even if stepper.distanceToGo() causes some I2C traffic, it will be
 * substantially less traffic than sending each stepper step seperately (see below).
 * If you want to cut down I2C polling completely, you can use the interrupt mechanism
 * (see interrupt example).
 */
void loop()
{
  stepper.runState(); // start the state machine with the set target and parameters
  while (stepper.distanceToGo() != 0) { // wait until target has been reached
    delay(250); // just to demonstrate that polling frequency is not critical
  }
  stepper.moveTo(-stepper.currentPosition());
}

/* This is the "classic" implementation which uses the original polling functions.
 * It will work, at least in low performance situations, but will clog the I2C bus as
 * each single (attempted) stepper step needs to be sent via I2C.
 */
void loopClassic()
{
  if (stepper.distanceToGo() == 0)
  { stepper.moveTo(-stepper.currentPosition()); }
  stepper.run(); // frequency is critical, but each call will cause I2C traffic...
}
