/*!
    @file ESP32sensorsI2C_firmware.h
    @brief Firmware module for the I2Cwrapper firmare.
    
    Allows I2C-control of touch buttons and hall sensorsof an ESP32.
    Meant mostly as a simple demo for user contributed modules using the
    interrupt mechanism. Will not compile for platforms other than ESP32.

    @section author Author
    Copyright (c) 2022 juh
    @section license License
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, version 2.
*/

#if MF_STAGE == MF_STAGE_includes
#include <ESP32sensorsI2C.h>
#include "esp32-hal-gpio.h"
#endif // MF_STAGE_includes


#if MF_STAGE == MF_STAGE_declarations
// It seems touchAttachInterruptArg() is not available yet (im March 2022), so we'll have to do this the hard way.
void touchDetected0() {
  triggerInterrupt(0, interruptReason_ESP32sensorsTouch);
}
void touchDetected1() {
  triggerInterrupt(1, interruptReason_ESP32sensorsTouch);
}
void touchDetected2() {
  triggerInterrupt(2, interruptReason_ESP32sensorsTouch);
}
void touchDetected3() {
  triggerInterrupt(3, interruptReason_ESP32sensorsTouch);
}
void touchDetected4() {
  triggerInterrupt(4, interruptReason_ESP32sensorsTouch);
}
void touchDetected5() {
  triggerInterrupt(5, interruptReason_ESP32sensorsTouch);
}
void touchDetected6() {
  triggerInterrupt(6, interruptReason_ESP32sensorsTouch);
}
void touchDetected7() {
  triggerInterrupt(7, interruptReason_ESP32sensorsTouch);
}
void touchDetected8() {
  triggerInterrupt(8, interruptReason_ESP32sensorsTouch);
}
void touchDetected9() {
  triggerInterrupt(9, interruptReason_ESP32sensorsTouch);
}
#endif // MF_STAGE_declarations


#if MF_STAGE == MF_STAGE_setup
log("ESP32sensorsI2C module enabled.\n");
#endif // MF_STAGE_setup



#if MF_STAGE == MF_STAGE_loop
#endif // MF_STAGE_loop



#if MF_STAGE == MF_STAGE_processMessage

case ESP32sensorsTouchSetCyclesCmd: {
  if (i == 4) { // uint16_t measure, uint16_t sleep
    uint16_t measure; bufferIn->read(measure);
    uint16_t sleep; bufferIn->read(sleep);
    touchSetCycles(measure, sleep);
    //bufferOut->write(++test);
  }
}
break;

case ESP32sensorsTouchReadCmd: {
  if (i == 1) { // uint8_t pin
    uint8_t pin; bufferIn->read(pin);
    bufferOut->write((uint16_t)touchRead(pin));
  }
}
break;

case ESP32sensorsEnableInterruptsCmd: {
  if (i == 4) { // uint8_t pin, uint16_t threshold, bool falling
    uint8_t pin; bufferIn->read(pin);
    uint16_t threshold; bufferIn->read(threshold);
    bool falling; bufferIn->read(falling);
    log("    enabling int for touch pin #"); log(pin);  log(" (=touch");
    // touchInterruptSetThresholdDirection(falling); // not available yet
    // we use digitalPinToTouchChannel() instead of T0...T9 consts, as they are not defined completely for all boards in pins_arduino.h
    // (see https://forum.arduino.cc/t/using-constants-that-might-not-be-declared-for-some-boards/975830 )
    switch (digitalPinToTouchChannel(pin)) { // this would be easier with touchAttachInterruptArg(), but it seems it's not yet available in ESP32 core
      case (0): { // smarter people could probably use an array of functions, but I can't
          touchAttachInterrupt(pin, touchDetected0, threshold);
          log("0"); 
          break;
        }
      case (1): {
          touchAttachInterrupt(pin, touchDetected1, threshold);
          log("1"); 
          break;
        }
      case (2): {
          touchAttachInterrupt(pin, touchDetected2, threshold);
          log("2"); 
          break;
        }
      case (3): {
          touchAttachInterrupt(pin, touchDetected3, threshold);
          log("3"); 
          break;
        }
      case (4): {
          touchAttachInterrupt(pin, touchDetected4, threshold);
          log("4"); 
          break;
        }
      case (5): {
          touchAttachInterrupt(pin, touchDetected5, threshold);
          log("5"); 
          break;
        }
      case (6): {
          touchAttachInterrupt(pin, touchDetected6, threshold);
          log("6"); 
          break;
        }
      case (7): {
          touchAttachInterrupt(pin, touchDetected7, threshold);
          log("7"); 
          break;
        }
      case (8): {
          touchAttachInterrupt(pin, touchDetected8, threshold);
          log("8"); 
          break;
        }
      case (9): {
          touchAttachInterrupt(pin, touchDetected9, threshold);
          log("9"); 
          break;
        }
    }
    log(") with threshold="); log(threshold); log("\n");
  }
}
break;

case ESP32sensorsHallReadCmd: {
  if (i == 0) { // no params
    bufferOut->write((int16_t)hallRead());
  }
}
break;

case ESP32sensorsTemperatureReadCmd: {
  if (i == 0) { // no params
    bufferOut->write((float)temperatureRead());
  }
}
break;

#endif // MF_STAGE_processMessage


#if MF_STAGE == MF_STAGE_reset
#endif // MF_STAGE_reset
