/*
   PinI2C Pin control demo
   (c) juh 2022

   Reads a digital and an analog input pin and mirrors their values on a
   digital and a PWM-capable output pin.
   Needs PinI2C.h module enabled in the target's firmware_modules.h.

*/


#include <Wire.h>
#include <PinI2C.h>

uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress); // each target device is represented by a wrapper...
PinI2C pins(&wrapper); // ...that the pin interface needs to communicate with the controller

/*
 * Arduino Uno/Nano example pins
 */

const uint8_t dPinIn  = 12; // any pin; connect switch against GND and +V (or use only GND and INPUT_PULLUP below)
const uint8_t dPinOut = 13; // any pin; connect LED with resistor or just use 13 = LED_BUILTIN on Uno/Nano
const uint8_t aPinIn  = 14; // needs analog pin; 14 = A0 on Uno/Nano; connect potentiometer against GND and +V
const uint8_t aPinOut = 6;  // needs PWM pin; 6 is PWM-capable on Uno/Nano; connect LED with resistor, or multimeter

/*  
 *  Attiny85/Digispark example pins
 *  
 *  The Digispark has 6 pins, two are needed for I2C (SDA=0/P0/PB0, SCL=2/P2/PB2).
 *  Pin 5/P5/PB5 is not usable as IO-pin on many clones without changing the fuses, so we have only three pins (1,3, and 4).
 *  Since this demo needs four, we'll use the same pin 3/P3/PB3 for digital *and* analog input:
 *  
const uint8_t dPinIn  = 3; // P3/PB3
const uint8_t dPinOut = 1; // P1/PB1 = built in LED on Digispark Model A
const uint8_t aPinIn  = 3; // P3/PB3; if we don't use a pullup, the analog value will float at ca. 700
const uint8_t aPinOut = 4; // P4/PB4 is the only remaining PWM pin, the others are needed as SDA and builtin LED
*/

/*  
 * Attiny88/MH-ET-live "HW-Tiny" example pins
 * (choose "MH Tiny" in ATTinyCore menu options for correct pin assignments)
 *  
const uint8_t dPinIn  = 14; // 14
const uint8_t dPinOut = 0;  // 0 = LED_BUILTIN
const uint8_t aPinIn  = 22; // 22 = A6
const uint8_t aPinOut = 10; // 10
*/

void setup()
{
  Wire.begin();
  Serial.begin(115200);

#ifdef SerialUSB
  // native USB needs to wait
  unsigned int begin_time = millis();
  while (! Serial && millis() - begin_time < 1000) {
    delay(10);  // but at most 1 sec if not plugged in to usb
  }
#endif

  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side

  if (!wrapper.ping()) {
    halt("Target not found! Check connections and restart.");
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }
  wrapper.reset(); // reset the target device

  pins.pinMode(dPinIn, INPUT); // INPUT_PULLUP will also work
  pins.pinMode(dPinOut, OUTPUT);
  pins.pinMode(aPinIn, INPUT);
  pins.pinMode(aPinOut, OUTPUT);

}

void loop()
{
  pins.digitalWrite(dPinOut, pins.digitalRead(dPinIn));
  pins.analogWrite(aPinOut, pins.analogRead(aPinIn) / 4);
  Serial.print("Digital input pin = "); Serial.print(pins.digitalRead(dPinIn));
  Serial.print(" | Analog input pin = "); Serial.println(pins.analogRead(aPinIn));
  delay(500);
}

void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}
