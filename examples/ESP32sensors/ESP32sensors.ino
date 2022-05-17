/*
   ESP32sensorsI2C demo
   (c) juh 2022

   Uses the I2Cwrapper interrupt mechanism to detect a touch button input
   on a ESP32 target device (without debouncing) and output touch button, 
   hall, and temperature readings.

   Needs ESP32sensorsI2C.h module enabled in the target's firmware_modules.h.
   
   Note: The temperature sensor seems not to be present or working on all 
   ESP32s. I could only get the 53.33 reading that's often mentioned in the net,
   even after torturing the device with a heat gun. Hall and touch sensors, work 
   flawlessly, though.

*/

#include <Wire.h>
#include <ESP32sensorsI2C.h>

// adapt as needed:
uint8_t i2cAddress = 0x08;
const uint8_t interruptPinTarget  = 26; // needs to be wired to interruptPinController
const uint8_t interruptPinController = 26; // wired to interruptPinTarget, needs to be a hardware interrupt pin (2 or 3 on Arduino Uno/Nano, any ESP32 pin)

const uint8_t touchPin   = 4; // ESP32 GPIO number of touch input T0
const uint8_t touchPinID = 0; // "0" for T0 (interrupt unit from clearInterrupt() is limited to 4 bit, that's why it returns the Id, not the pin number)

I2Cwrapper wrapper(i2cAddress); // each target device is represented by a wrapper...
ESP32sensorsI2C sensors(&wrapper); // ...that the pin interface needs to communicate with the target
volatile bool interruptFlag = false; // volatile for interrupt

// ISR
// this has to come before setup(), else the Arduino environment will be confused by the conditionals and macros
#if defined(ESP8266) || defined(ESP32)
void IRAM_ATTR interruptFromTarget()
#else
void interruptFromTarget()
#endif
{
  interruptFlag = true; // just set a flag, leave interrupt as quickly as possible
}


void setup()
{
  Wire.begin();
  Serial.begin(115200);
  Serial.println("\n\nESP32 I2C sensors demo\n");
//  ###
  //Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side
  wrapper.setI2Cdelay(25);

  if (!wrapper.ping()) {
    halt("Target not found! Check connections and restart.");
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }

  wrapper.reset(); // reset the target device
  delay(500); // and give it time to reboot

  // First, make the target send interrupts. The interrupt pin is shared by all target modules
  // which use interrupts, so we need the wrapper to set it up.
  wrapper.setInterruptPin(interruptPinTarget, /* activeHigh */ true); // activeHigh -> controller will have to look out for a RISING flank
  sensors.enableInterrupts(touchPin, 40); // make target send out interrupts for this pin at the interupt pin set above with threshold 40
  
  // make the controller listen to the interrupt pin
  pinMode(interruptPinController, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPinController), interruptFromTarget, RISING);

}

unsigned long then = millis();
void loop()
{
  if (interruptFlag) {
    interruptFlag = false;
    uint8_t ru = wrapper.clearInterrupt();
    uint8_t triggerReason = ru >> 4;
    uint8_t triggerUnit = ru & 0xf;
    if (triggerReason == interruptReason_ESP32sensorsTouch and triggerUnit == touchPinID) {
      Serial.print("\nInterrupt from touch button T"); Serial.print(triggerUnit);
      Serial.print(": Touch T0 = "); Serial.print(sensors.touchRead(touchPin));;
      Serial.print(" | hall sensor = "); Serial.print(sensors.hallRead());
      Serial.print(" | temperature (probably wrong) = "); Serial.println(sensors.temperatureRead());      
    } else {
      Serial.print("\n\nUnexpected interrupt from unit "); Serial.print(triggerUnit);
      Serial.print(" with interrupt reason "); Serial.println(triggerReason);
    }
  }
  //  use this for debugging:
  //  if (millis()-then > 1000) {
  //    Serial.print(digitalRead(interruptPinController));
  //    Serial.print("  "); 
  //    Serial.println(sensors.touchRead(touchPin));
  //    then = millis();
  //  }
}



void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}
