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

    @todo touchAttachInterruptArg() is available since May 2022 with
    esp32 release 2.0.3. Implement it for a much easier and cleaner solution.
    Also implement touchInterruptSetThresholdDirection()
    Also adapt for SOC_TOUCH_SENSOR_NUM no. of sensors (I think newer ESP32 variants have more than 10)
*/

#if !defined(ARDUINO_ARCH_ESP32)
#error "ESP32sensorsI2C_firmware.h needs an ESP32 device."
#endif

/*
   (1) includes
*/

#if MF_STAGE == MF_STAGE_includes
#include <ESP32sensorsI2C.h>
//#include <esp32-hal-gpio.h>
//#include <esp32-hal-touch.c>
#include <soc/touch_sensor_periph.h> // needed for touch_sensor_channel_io_map[] in reset code
#include <driver/touch_sensor.h> // needed for touch_pad_get_meas_time() for reset code
#endif // MF_STAGE_includes


/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations
// backup stuff for restore code
uint16_t isAttached = 0; // bit positions 0-9 used for logging attached pins, so we can detach them in reset code
uint16_t ESP32sensorsTouchSetCycles_old_sleep_cycle, ESP32sensorsTouchSetCycles_old_meas_cycle;
bool ESP32sensorsTouchSetCyclesChanged = false;

// It seems touchAttachInterruptArg() is not available yet (in March 2022), so we'll have to do this the hard way.
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


/*
   (3) setup() function
*/

#if MF_STAGE == MF_STAGE_setup
log("ESP32sensorsI2C module enabled.\n");
#endif // MF_STAGE_setup


/*
   (4) main loop() function
*/

#if MF_STAGE == MF_STAGE_loop
#endif // MF_STAGE_loop


/*
   (5) processMessage() function
*/

#if MF_STAGE == MF_STAGE_processMessage

case ESP32sensorsTouchSetCyclesCmd: {
  if (i == 4) { // uint16_t measure, uint16_t sleep
    uint16_t measure; bufferIn->read(measure);
    uint16_t sleep; bufferIn->read(sleep);
    if (!ESP32sensorsTouchSetCyclesChanged) { // store initial values for later restore in reset code
      touch_pad_get_meas_time(&ESP32sensorsTouchSetCycles_old_sleep_cycle, &ESP32sensorsTouchSetCycles_old_meas_cycle);
      ESP32sensorsTouchSetCyclesChanged = true;
    }
    touchSetCycles(measure, sleep);
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
          isAttached |= 1 << 0;
          log("0");
          break;
        }
      case (1): {
          touchAttachInterrupt(pin, touchDetected1, threshold);
          isAttached |= 1 << 1;
          log("1");
          break;
        }
      case (2): {
          touchAttachInterrupt(pin, touchDetected2, threshold);
          isAttached |= 1 << 2;
          log("2");
          break;
        }
      case (3): {
          touchAttachInterrupt(pin, touchDetected3, threshold);
          isAttached |= 1 << 3;
          log("3");
          break;
        }
      case (4): {
          touchAttachInterrupt(pin, touchDetected4, threshold);
          isAttached |= 1 << 4;
          log("4");
          break;
        }
      case (5): {
          touchAttachInterrupt(pin, touchDetected5, threshold);
          isAttached |= 1 << 5;
          log("5");
          break;
        }
      case (6): {
          touchAttachInterrupt(pin, touchDetected6, threshold);
          isAttached |= 1 << 6;
          log("6");
          break;
        }
      case (7): {
          touchAttachInterrupt(pin, touchDetected7, threshold);
          isAttached |= 1 << 7;
          log("7");
          break;
        }
      case (8): {
          touchAttachInterrupt(pin, touchDetected8, threshold);
          isAttached |= 1 << 8;
          log("8");
          break;
        }
      case (9): {
          touchAttachInterrupt(pin, touchDetected9, threshold);
          isAttached |= 1 << 9;
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


/*
   (6) reset event
*/

#if MF_STAGE == MF_STAGE_reset

// restore setCycles
if (ESP32sensorsTouchSetCyclesChanged) {
  touchSetCycles(ESP32sensorsTouchSetCycles_old_sleep_cycle, ESP32sensorsTouchSetCycles_old_meas_cycle);
}

// remove interrupts
for (uint8_t i = 0; i < SOC_TOUCH_SENSOR_NUM; i++) { // cycle through all touch channels
  if ((isAttached & 1 << i) != 0) { // touch channel has been attached
    touchDetachInterrupt(touch_sensor_channel_io_map[i]);
  }
}
#endif // MF_STAGE_reset
