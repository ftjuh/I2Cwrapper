/*
   AccelStepper I2C demo for CNC shield V3.00
   (c) juh 2022

   1. install library to Arduino environment
   2. upload firmware.ino to Arduino UNO with CNC V3.00 shield (slave)
   3. upload this example to another Arduino-like (master), I used a Wemos D1 mini with ESP8266
   4. connect the I2C bus of both devices (usually A4<>A4, A5<>A5, GND<>GND)
   5. don't fortget two I2C pullups and level-shifters if needed
   6. also connect +5V<>+5V to power one board from the other, if needed
   7. connect steppers (and Stepstick drivers, of course) to axes X, Y, and Z
   8. now (not earlier) provide external power to the steppers and power to the Arduinos
   9. smile.
*/

//#include <AccelStepper.h>

#include <Arduino.h>
#include <AccelStepperI2C.h>
#include <Wire.h>

const uint8_t addr = 0x8; // i2c address of slave
const uint8_t endstopXpin = 9;

AccelStepperI2C* X; // do not invoke the constructors here, as we need to establish I2C connection first. 
AccelStepperI2C* Y; // 
AccelStepperI2C* Z;

void printStats(AccelStepperI2C* O) {
  Serial.print("currentPosition()= "); Serial.print(O->currentPosition());
  Serial.print("  targetPosition()= "); Serial.print(O->targetPosition());
  Serial.print("  distanceToGo()= "); Serial.print(O->distanceToGo());
  Serial.print("  speed()= "); Serial.print(O->speed());
  Serial.print("  maxSpeed()= "); Serial.println(O->maxSpeed());
}

void testDryRun() {
  Serial.print("Let's do some dry run tests, first.\n\n");
  delay(2000);

  Serial.print("\nTesting target/position related commands:\n1. set position to 500000 (long): setCurrentPosition(500000)");
  X->setCurrentPosition(500000);
  printStats(X); delay(2000);

  Serial.print("\n2. absolute movement (long): moveTo(500500)");
  X->moveTo(500500);
  printStats(X); delay(2000);

  Serial.print("\n3. relative movement (long): move(-500500)");
  X->move(-500500);
  printStats(X); delay(2000);

  Serial.print("\n\nTesting speed related commands:\n4. set speed limit (float): setMaxSpeed(345.34)");
  X->setMaxSpeed(345.34);
  printStats(X); delay(2000);

  Serial.print("\n5. set speed (float): setSpeed(23.43)");
  X->setSpeed(23.43);
  printStats(X); delay(2000);

}
void setup() {

  // Important: initialize Wire before creating AccelStepperI2C objects
  Wire.begin();

  // If the slave's and master's reset is not synchronized by hardware, after a master's reset the slave might
  // think the master wants another stepper, not a first one, and will run out of steppers, sooner or later.
  resetAccelStepperSlave(addr);

  Serial.begin(115200);
  delay(200);
  Serial.print("\n\nAccelStepperI2C demo - CNC Shield V3.00\n\n\n");
  Serial.flush();
  delay(1000);

  X = new AccelStepperI2C(addr, AccelStepper::DRIVER, /* step pin */ 2, /* dir pin */ 5);
  Y = new AccelStepperI2C(addr, AccelStepper::DRIVER, 3, 6, 0, 0, true);
  Z = new AccelStepperI2C(addr, AccelStepper::DRIVER, 4, 7, 0, 0, true);
  // you could add stepper A on pins 12 (step) and 13 (dir) after installing 
  // the two jumpers directly above the power terminal
  
  if ((X->myNum == -1) or (Y->myNum == -1) or (Z->myNum == -1)) {
    Serial.print("Error: Could not add one or more steppers. Halting\n\n");
    while (true) {}
  }
  Serial.print("Steppers added with internal numbers X = ");
  Serial.print(X->myNum); Serial.print(", Y = ");
  Serial.print(Y->myNum); Serial.print(", Z = ");
  Serial.print(Z->myNum); Serial.print(".\n\n");

  /* The CNC shield by default pulls the four drivers' _EN_ pins high, making them inactive.
     You can enable theim either by installing a jumper next to the reset button between EN and GND
     *or* by software. As all four EN are connected to Arduino pin 8, we have to tell Accelstepper
     about that pin and to call enableOutputs(). Before that, we have to tell it that _EN_ is active low
     with setPinsInverted().
  */
  X->setEnablePin(8); 
  X->setPinsInverted(false, false, true); // directionInvert, stepInvert, enableInvert
  X->enableOutputs(); 
  Y->setEnablePin(8); 
  Y->setPinsInverted(false, false, true);
  Y->enableOutputs(); 
  Z->setEnablePin(8); 
  Z->setPinsInverted(false, false, true);
  Z->enableOutputs();

  // tailor MaxSpeeds and acceleration to your steppers, here X is a Nema-14, Y and Z are geared 28BYJ-48's
  X->setMaxSpeed(2000); Y->setMaxSpeed(500); Z->setMaxSpeed(500); 
  X->setAcceleration(200); Y->setAcceleration(300); Z->setAcceleration(300);

  // install endstop switch 
  X->setEndstopPin(endstopXpin, true, true);  // activeLow works nicely together with internal pullup
  X->setEndstopPin(10, true, true);  // activeLow works nicely together with internal pullup
  X->enableEndstops(); // true by default
  
  X->moveTo(5000); Y->moveTo(8000); Z->moveTo(12000); // set targets
  X->runState(); Y->runState(); Z->runState(); // start the three state machines

  // X->setSpeed(500); Y->setSpeed(500); Z->setSpeed(500);
  //X->runSpeedState(); Y->runSpeedState(); Z->runSpeedState();

  //  while (true)   {
  //    ESP.wdtFeed();
  //    yield(); // prevent ESP watchdog reset
  //  }
  //  X->setMaxSpeed(100); X->setAcceleration(20); X->moveTo(500);
  //  Y->setMaxSpeed(100); Y->setAcceleration(20); Y->moveTo(500);
  //  Z->setMaxSpeed(100); Z->setAcceleration(20); Z->moveTo(500);
}

void loop() {
  Serial.print("X endstops="); Serial.print(X->endstops()); Serial.print("  - "); printStats(X); //Serial.print("Y - "); printStats(Y); Serial.print("Z - "); printStats(Z);
  delay (1000);
  if (abs(X->distanceToGo()) == 0) {
    X->moveTo(0 - X->currentPosition()); 
    // after the target has been reached, the state machine resets itself to state_stopped. So we must start it again.
    X->runState(); 
  }

  if (abs(Y->distanceToGo()) == 0) {
    Y->moveTo(0 - Y->currentPosition()); Y->runState();
  }

  if (abs(Z->distanceToGo()) == 0) {
    Z->moveTo(0 - Z->currentPosition()); Z->runState();
  }

}

/* test protocol
ok    void    moveTo(long absolute);
ok    void    move(long relative);
ok    boolean run();
ok    boolean runSpeed();
ok    void    setMaxSpeed(float speed);
ok    float   maxSpeed();
ok    void    setAcceleration(float acceleration);
ok    void    setSpeed(float speed);
ok    float   speed();
ok    long    distanceToGo();
ok    long    targetPosition();
ok    long    currentPosition();
    void    setCurrentPosition(long position);
    void    runToPosition();
    boolean runSpeedToPosition();
    void    runToNewPosition(long position);
    void stop();
    virtual void    disableOutputs();
ok    virtual void    enableOutputs();
    void    setMinPulseWidth(unsigned int minWidth);
ok    void    setEnablePin(uint8_t enablePin = 0xff);
ok    void    setPinsInverted(bool directionInvert = false, bool stepInvert = false, bool enableInvert = false);
    void    setPinsInverted(bool pin1Invert, bool pin2Invert, bool pin3Invert, bool pin4Invert, bool enableInvert);
    bool    isRunning();

*/
