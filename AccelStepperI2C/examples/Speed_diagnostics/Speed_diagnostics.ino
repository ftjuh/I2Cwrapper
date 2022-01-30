/*
  AccelStepperI2C speed tests
  (c) juh 2022
  
  Generate the data for the performance graphs included in the documentation.
  Copy and paste the output to a spreadsheet for further analysis.
  
  1. install library to Arduino environment
  2. in firmware.ino, enable (uncomment) DIAGNOSTICS_AccelStepperI2C and disable (comment)
     DEBUG_AccelStepperI2C. Upload firmware.ino to testing platform.
  4. upload this example to another Arduino-like (master), I used a Wemos D1 mini with ESP8266
  4. connect the I2C bus of both devices (usually A4<>A4, A5<>A5, GND<>GND)
  5. don't fortget two I2C pullups and level-shifters if needed
  6. also connect +5V<>+5V to power one board from the other, if needed
  7. Don't connect drivers and steppers for these speed tests.
*/

#include <Arduino.h>
#include <AccelStepperI2C.h>
#include <Wire.h>

//#define DEBUG_AccelStepperI2C

const uint8_t addr = 0x8; // i2c address of slave

const byte maxSteppers = 8;
AccelStepperI2C* S[maxSteppers];
byte numSteppers = 0;

diagnosticsReport* report;
diagnosticsReport* report0;
diagnosticsReport* report1;

void setup() {

  // Important: initialize Wire before creating AccelStepperI2C objects
  Wire.begin();
  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println("\n\n\nAccelStepperI2C demo - Speed tests\n\n");

  // If the slave's and master's reset is not synchronized by hardware, after a master's reset the slave might
  // think the master wants another stepper, not a first one, and will run out of steppers, sooner or later.
  Serial.print("\n\nresetting slave\n");
  resetAccelStepperSlave(addr);
  delay(500);

  report = new diagnosticsReport;
  report0 = new diagnosticsReport;
  report1 = new diagnosticsReport;

  Serial.println("\n\nMeasuring processing times (in milliseconds)\n\n");

  for (byte i = 0; i < maxSteppers; i++) {
    S[i] = new AccelStepperI2C(addr, AccelStepper::DRIVER, /* step pin */ 4, /* dir pin */ 5);
    //Serial.print("\nStepper added with no. "); Serial.println(S[i]->myNum);
    Serial.print(S[i]->myNum + 1); Serial.println(" steppers attached");

    numSteppers++;
    if (i == 0) {
      S[0]->enableDiagnostics(); // call only once
    }
    testThem(1000);
  }

}

uint32_t countCycles(int millis) {
  S[0]->diagnostics(report0); // make cycles reset, so we don't risk an overrun
  S[0]->diagnostics(report0);
  delay(millis);
  S[0]->diagnostics(report1);
  return ((report1->cycles) - (report0->cycles));
}

void resetStepper(uint8_t stp) {
  S[stp]->stopState();
  S[stp]->setSpeed(0);
  S[stp]->setCurrentPosition(0);
  S[stp]->moveTo(0);
  S[stp]->setAcceleration(0);
}

void resetSteppers() {
  for (byte i = 0; i < numSteppers; i++) {
    resetStepper(i);
  }
  delay(100);
}

void testThem(uint32_t ms) {
  resetSteppers();

  Serial.print("runSpeed: ");
  for (byte i = 0; i < maxSteppers; i++) {
    if (i < numSteppers) {
      S[i]->setMaxSpeed(1000); S[i]->setSpeed(1000);
      S[i]->runSpeedState();
      Serial.print(countCycles(ms)); Serial.print(" ");
    } else {
      Serial.print(0); Serial.print(" ");
    }
  }
  Serial.println();
  resetSteppers();

  Serial.print("runSpeedToPosition: ");
  for (byte i = 0; i < maxSteppers; i++) {
    if (i < numSteppers) {
      S[i]->setMaxSpeed(1000); S[i]->setSpeed(1000);
      S[i]->moveTo(1000000);
      S[i]->runSpeedToPositionState();
      Serial.print(countCycles(ms)); Serial.print(" ");
    } else {
      Serial.print(0); Serial.print(" ");
    }
  }
  Serial.println();
  resetSteppers();

  Serial.print("run: ");
  for (byte i = 0; i < maxSteppers; i++) {
    if (i < numSteppers) {
      S[i]->setAcceleration(100.0); S[i]->moveTo(1000000);
      S[i]->runState();
      Serial.print(countCycles(ms)); Serial.print(" ");
    } else {
      Serial.print(0); Serial.print(" ");
    }
  }
  Serial.println();
  resetSteppers();

}

// use this to get an idea how long the slave spends in onRequest() and onReceive() interrupts
// and how long it takes to process a command from the master
void processingTimes() {
  S[0]->diagnostics(report);
  Serial.print("lastProcessTime = "); Serial.println(report->lastProcessTime);
  Serial.print("lastRequestTime = "); Serial.println(report->lastRequestTime);
  Serial.print("lastReceiveTime = "); Serial.println(report->lastReceiveTime);
}

void loop() {

}
