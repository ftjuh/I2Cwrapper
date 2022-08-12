/*!
   @file ServoI2C_firmware.h
   @brief Firmware module for the I2Cwrapper firmare.

   Provides I2C access to servo motors connected to the target device.
   Mimicks the standard Servo library functions.

   ## Author
   Copyright (c) 2022 juh
   ## License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @note At the moment, this will only work if controller and target use the same values
   for sizeof(int).
   @todo Need to typecast the transmission of int values to int16_t.
*/


/*
   (1) includes
*/

#if MF_STAGE == MF_STAGE_includes
#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD)
#include <Servo.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <ESP32Servo.h> // ESP32 doesn't come with a native Servo.h
#endif // defined(ARDUINO_ARCH_AVR)
#include <ServoI2C.h>
#endif // MF_STAGE_includes


/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations
const uint8_t maxServos = 4;
Servo servos[maxServos]; // @todo this should be an array of pointers to Servo objects
uint8_t numServos = 0; // number of initialised servos.

bool validServo(int8_t s)
{
  return (s >= 0) and (s < numServos);
}
#endif // MF_STAGE_declarations


/*
   (3) setup() function
*/

#if MF_STAGE == MF_STAGE_setup
log("ServoI2C module enabled.\n");
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

/*
   Note: servo implementation is dumb. It passes each command over I2C even if only some of the commands
   actually do something target related. It would be much smarter to let the controller do commands like
   attached() without I2C traffic. The dumb way however is easier and safer.
*/

case servoAttach1Cmd: {
  if ((numServos < maxServos) and (i == 2)) { // 1 int
    int p; bufferIn->read(p);
    servos[numServos].attach(p);
    bufferOut->write(numServos++);
    log(numServos);
  }
}
break;

case servoAttach2Cmd: {
  if ((numServos < maxServos) and (i == 6)) { // 3 int
    int p; bufferIn->read(p);
    int min; bufferIn->read(min);
    int max; bufferIn->read(max);
    servos[numServos].attach(p, min, max);
    bufferOut->write(numServos++);
  }
}
break;

case servoDetachCmd: {
  if (validServo(unit) and (i == 0)) { // no parameters
    servos[unit].detach();
  }
}
break;

case servoWriteCmd: {
  if (validServo(unit) and (i == 2)) { // 1 int
    int value; bufferIn->read(value);
    servos[unit].write(value);
  }
}
break;

case servoWriteMicrosecondsCmd: {
  if (validServo(unit) and (i == 2)) { // 1 int
    int value; bufferIn->read(value);
    servos[unit].writeMicroseconds(value);
  }
}
break;

case servoReadCmd: {
  if (validServo(unit) and (i == 0)) { // no parameters
    bufferOut->write(servos[unit].read());
  }
}
break;

case servoReadMicrosecondsCmd: {
  if (validServo(unit) and (i == 0)) { // no parameters
    bufferOut->write(servos[unit].readMicroseconds());
  }
}
break;

case servoAttachedCmd: {
  if (validServo(unit) and (i == 0)) { // no parameters
    bufferOut->write(servos[unit].attached());
  }
}
break;

#endif // MF_STAGE_processMessage


/*
   (6) reset event
*/

#if MF_STAGE == MF_STAGE_reset

for (uint8_t i = 0; i < numServos; i++) {
  servos[i].detach();
}
numServos = 0;

#endif // MF_STAGE_reset
