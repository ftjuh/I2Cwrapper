/*
   PinI2C Pin control demo
   (c) juh 2022

   Reads a digital and an anolog pin and mirrors both on another 
   digital pin and a PWM-capable pin.
   Needs PINCONTROL_SUPPORT enabled in the slave's firmware.

*/


#include <Wire.h>
#include <PinI2C.h>

uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress); // each slave device is represented by a wrapper...
PinI2C pins(&wrapper); // ...that the pin interface needs to communicate with the slave

const uint8_t dPinIn  = 12; // any pin; connect switch against GND and +V
const uint8_t dPinOut = 13; // any pin; connect LED with resistor or just use 13 = LED_BUILTIN on Uno/Nano 
const uint8_t aPinIn  = 14; // needs analog pin; 14 = A0 on Uno/Nano; connect potentiometer against GND and +V
const uint8_t aPinOut = 6;  // needs PWM pin; 6 is PWM-capable on Uno/Nano; connect LED with resistor, or multimeter


void setup()
{
  Wire.begin();
  Serial.begin(115200);
  // Wire.setClock(10000); // uncomment for ESP8266 slaves, to be on the safe side
  
  if (!wrapper.ping()) {
    halt("Slave not found! Check connections and restart.");
  } else {
    Serial.println("Slave found as expected. Proceeding.\n");
  }
  
  wrapper.reset(); // reset the slave device
  delay(500); // and give it time to reboot

  pins.pinMode(dPinIn, INPUT); // INPUT_PULLUP will also work
  pins.pinMode(dPinOut, OUTPUT);
  pins.pinMode(aPinIn, INPUT);
  pins.pinMode(aPinOut, OUTPUT);
  
}

void loop()
{
  pins.digitalWrite(dPinOut, pins.digitalRead(dPinIn));
  pins.analogWrite(aPinOut, pins.analogRead(aPinIn)/4);
  Serial.print("Digital input pin = "); Serial.print(pins.digitalRead(dPinIn));
  Serial.print(" | Analog input pin = "); Serial.println(pins.analogRead(aPinIn));
  delay(500);
}

void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) { yield();} // prevent ESPs from watchdog reset
}
