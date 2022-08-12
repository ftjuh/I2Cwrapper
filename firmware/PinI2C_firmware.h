/*!
    @file PinI2C_firmware.h
    @brief Firmware module for the I2Cwrapper firmare.

    Provides access to the target's analog and digital input and output pins.
    Mimicks the standard Arduino functions like pinMode(), digitalRead(), etc.

    ## Author
    Copyright (c) 2022 juh
    ## License
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, version 2.
*/


/*
   (1) includes
*/

#if MF_STAGE == MF_STAGE_includes
#include <PinI2C.h>
#endif


/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations

// we need to track all used pins to be able to reset them if needed
uint8_t usedPins[NUM_DIGITAL_PINS]; // number of available pins on this platform
uint8_t numUsedPins = 0;

#endif


/*
   (3) setup() function
*/

#if MF_STAGE == MF_STAGE_setup
log("PinI2C module enabled.\n");
#endif


/*
   (4) main loop() function
*/

#if MF_STAGE == MF_STAGE_loop
#endif


/*
   (5) processMessage() function
*/

#if MF_STAGE == MF_STAGE_processMessage

case pinPinModeCmd: {
  if (i == 2) { // 2 uint8_t
    uint8_t pin; bufferIn->read(pin);
    uint8_t mode; bufferIn->read(mode);
    log("pinMode("); log(pin); log(", "); log(mode); log(")\n\n");
    pinMode(pin, mode);
    usedPins[numUsedPins++] = pin; // track this pin to be able to reset it later
    numUsedPins %= NUM_DIGITAL_PINS; // in case the controller allocates more pins than available...
  }
}
break;

case pinDigitalReadCmd: {
  if (i == 1) { // 1 uint8_t
    uint8_t pin; bufferIn->read(pin);
    bufferOut->write((int16_t)digitalRead(pin)); // int is not 2 bytes on all Arduinos (sigh)
  }
}
break;

case pinDigitalWriteCmd: {
  if (i == 2) { // 2 uint8_t
    uint8_t pin; bufferIn->read(pin);
    uint8_t value; bufferIn->read(value);
    digitalWrite(pin, value);
  }
}
break;

case pinAnalogReadCmd: {
  if (i == 1) { // 1 uint8_t
    uint8_t pin; bufferIn->read(pin);
    bufferOut->write((int16_t)analogRead(pin)); // int is not 2 bytes on all Arduinos (sigh)
  }
}
break;

case pinAnalogWriteCmd: {
  if (i == 3) { // 1 uint8_t, 1 int16_t ("int")
    uint8_t pin; bufferIn->read(pin);
    int16_t value = 0; bufferIn->read(value);
    analogWrite(pin, value);
  }
}
break;

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAMD) // ESPs don't have analogReference()
case pinAnalogReferenceCmd: {
  if (i == 1) { //1 uint8_t
    uint8_t mode; bufferIn->read(mode);
#ifdef ARDUINO_ARCH_SAMD
    analogReference(static_cast<eAnalogReference>(mode));
#elif defined(ARDUINO_ARCH_AVR)
    analogReference(mode);
#else
      log("Warning: analogReference() is not supported on this architecture.\n");
#endif
  }
}
break;
#endif // defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAMD)

#endif // stage


/*
   (6) reset event
*/

#if MF_STAGE == MF_STAGE_reset

for (uint8_t i = 0; i < numUsedPins; i++) {
  pinMode(usedPins[i], INPUT); // pin default state (https://www.arduino.cc/en/Tutorial/Foundations/DigitalPins), will also turn it off, if it was high output before
}
numUsedPins = 0;

#endif // MF_STAGE_reset
