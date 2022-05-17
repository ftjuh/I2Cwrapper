/*!
 *  @file PinI2C_firmware.h
 *  @brief Firmware module for the I2Cwrapper firmare.
 *  
 *  Provides access to the target's analog and digital input and output pins.
 *  Mimicks the standard Arduino functions like pinMode(), digitalRead(), etc.
 * 
 *  @section author Author
 *  Copyright (c) 2022 juh
 *  @section license License
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, version 2.
 */

#if MF_STAGE == MF_STAGE_includes
#include <PinI2C.h>
#endif


#if MF_STAGE == MF_STAGE_declarations
#endif


#if MF_STAGE == MF_STAGE_setup
  log("PinI2C module enabled.\n");
#endif


#if MF_STAGE == MF_STAGE_loop
#endif


#if MF_STAGE == MF_STAGE_processMessage

      case pinPinModeCmd: {
          if (i == 2) { // 2 uint8_t
            uint8_t pin; bufferIn->read(pin);
            uint8_t mode; bufferIn->read(mode);
            log("pinMode("); log(pin); log(", "); log(mode); log(")\n\n");
            pinMode(pin, mode);
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
            int16_t value; bufferIn->read(value);
            analogWrite(pin, value);
          }
        }
        break;

#if defined(ARDUINO_ARCH_AVR) // ESPs don't have analogReference()
      case pinAnalogReferenceCmd: {
          if (i == 1) { //1 uint8_t
            uint8_t mode; bufferIn->read(mode);
            analogReference(mode);
          }
        }
        break;
#endif // defined(ARDUINO_ARCH_AVR)


#endif
