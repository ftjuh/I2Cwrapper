//
//   Whizzbizz.com - 2022
//   AvD on march 29th 2022 - Test three steppers on the CNC v4 board over I2C with AccelStepperI2C...
//

#include <Wire.h>
#include <AccelStepperI2C.h>

//#define DEBUG 1

// Specific defines for CNC shield V4 with Arduino Nano on the board
// Board info: https://www.chippiko.com/2020/11/tutorial-keyes-cnc-shield.html
// Attention: https://www.instructables.com/Fix-Cloned-Arduino-NANO-CNC-Shield/
//
#define MOTOR_X_ENABLE_PIN 8
#define MOTOR_X_STEP_PIN   5
#define MOTOR_X_DIR_PIN    2
 
#define MOTOR_Y_ENABLE_PIN 8
#define MOTOR_Y_STEP_PIN   6
#define MOTOR_Y_DIR_PIN    3
 
#define MOTOR_Z_ENABLE_PIN 8
#define MOTOR_Z_STEP_PIN   7
#define MOTOR_Z_DIR_PIN    4

#define defMaxSpeed     2000
#define defAcceleration 750
#define stepsToGo       200 // One turn on Nema14

uint8_t i2cAddress = 0x08;

// Each I2C driver target device is represented by a wrapper
I2Cwrapper wrapper(i2cAddress);
AccelStepperI2C stepperX(&wrapper); // Stepper Motor 1
AccelStepperI2C stepperY(&wrapper); // Stepper Motor 2
AccelStepperI2C stepperZ(&wrapper); // Stepper Motor 3

bool stepperXdir = true; // true=CW, false=CCW
bool stepperYdir = true;
bool stepperZdir = true;

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

  wrapper.setI2Cdelay(20); // Set small delay for I2C bus communication

  // attach() replaces the AccelStepper constructor, so it could also be called like this: 
  // stepper.attach(AccelStepper::DRIVER, 5, 6);
  // stepper.attach(); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
  stepperX.attach(AccelStepper::DRIVER, MOTOR_X_STEP_PIN, MOTOR_X_DIR_PIN); // Stepper Motor 1 - X
  stepperY.attach(AccelStepper::DRIVER, MOTOR_Y_STEP_PIN, MOTOR_Y_DIR_PIN); // Stepper Motor 2 - Y
  stepperZ.attach(AccelStepper::DRIVER, MOTOR_Z_STEP_PIN, MOTOR_Z_DIR_PIN); // Stepper Motor 3 - Z

#ifdef DEBUG  
  Serial.print("stepperX.myNum=");
  Serial.println(stepperX.myNum);
  Serial.print("stepperY.myNum=");
  Serial.println(stepperY.myNum);
  Serial.print("stepperZ.myNum=");
  Serial.println(stepperZ.myNum);
#endif

  if (stepperX.myNum < 0) { // Should not happen after a reset
    Serial.println("Error: stepperX could not be allocated");
    while (true) {}
  }
  if (stepperY.myNum < 0) { // Should not happen after a reset
    Serial.println("Error: stepperY could not be allocated");
    while (true) {}
  }
  if (stepperZ.myNum < 0) { // Should not happen after a reset
    Serial.println("Error: stepperZ could not be allocated");
    while (true) {}
  }
    
  // Change these to suit your stepper if you want...
  stepperX.setMaxSpeed(defMaxSpeed);
  stepperX.setAcceleration(defAcceleration);

  stepperY.setMaxSpeed(defMaxSpeed);
  stepperY.setAcceleration(defAcceleration);
  
  stepperZ.setMaxSpeed(defMaxSpeed);
  stepperZ.setAcceleration(defAcceleration);
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
  // AvD 2022-03-28 - ToDo: Experiment with interrupts for end-stops and to eliminate distanceToGo() polling I2C traffic...
  //
  if (stepperX.distanceToGo() == 0) { // Give stepper something to do...
    if (stepperXdir)
      stepperX.moveTo(stepsToGo);
    else
      stepperX.moveTo(-stepsToGo);
    stepperX.runState();  
    stepperXdir = !stepperXdir;
  }

  if (stepperY.distanceToGo() == 0) { // Give stepper something to do...
    if (stepperYdir)
      stepperY.moveTo(stepsToGo);
    else
      stepperY.moveTo(-stepsToGo);    
    stepperY.runState();  
    stepperYdir = !stepperYdir;
  }

  if (stepperZ.distanceToGo() == 0) { // Give stepper something to do...
    if (stepperZdir)
      stepperZ.moveTo(stepsToGo);
    else
      stepperZ.moveTo(-stepsToGo);
    stepperZ.runState(); 
    stepperZdir = !stepperZdir;
  }
    
  delay(1000); // Just slow down the main loop a bit...
}

/* This is the "classic" implementation which uses the original polling functions.
 * It will work, at least in low performance situations, but will clog the I2C bus as
 * each single (attempted) stepper step needs to be sent via I2C.
 */
void loopClassic() {
  if (stepperX.distanceToGo() == 0)
  { stepperX.moveTo(-stepperX.currentPosition()); }
  stepperX.run(); // frequency is critical, but each call will cause I2C traffic...
}
