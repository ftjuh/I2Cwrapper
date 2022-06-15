/*!
   @file TM1638liteI2C_firmware.h
   @brief TM1638liteI2C firmware module. See TM1638liteI2C.h for details.
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
   @todo Implement interrupt mechanism for button presses like ESP32sensors.
*/

/// @cond

/*
   (1) includes
*/
#if MF_STAGE == MF_STAGE_includes
#include <TM1638liteI2C.h>
#include <TM1638lite.h>
#endif // MF_STAGE_includes


/*
   (2) declarations
*/
#if MF_STAGE == MF_STAGE_declarations
const int8_t maxTM1638s = 4;
TM1638lite* TM1638s[maxTM1638s];
int8_t numTM1638s = 0; // number of initialised devices.

bool validTM1638(int8_t s)
{
  return (s >= 0) and (s < numTM1638s);
}
#endif // MF_STAGE_declarations


/*
   (3) setup() function
*/
#if MF_STAGE == MF_STAGE_setup
log("TM1638liteI2C module enabled.\n");
#endif // MF_STAGE_setup


/*
   (4) loop() function
*/
#if MF_STAGE == MF_STAGE_loop
#endif // MF_STAGE_loop


/*
   (5) processMessage() function
*/
#if MF_STAGE == MF_STAGE_processMessage

case TM1638liteAttachCmd: {
  if (i == 3) {
    if (numTM1638s < maxTM1638s) {
      uint8_t strobe;
      uint8_t clock;
      uint8_t data;
      bufferIn->read(strobe);
      bufferIn->read(clock);
      bufferIn->read(data);
      TM1638s[numTM1638s] = new TM1638lite(strobe, clock, data);
      bufferOut->write(numTM1638s++);
    } else {
      log("-- Too many TM1638 devices, failed to add new one\n");
      bufferOut->write(int8_t(-1));
    }
  }
}
break;

case TM1638liteSendCommandCmd: {
  if (validTM1638(unit) and (i == 1)) {
    uint8_t value;
    bufferIn->read(value);
    TM1638s[unit]->sendCommand(value);
  }
}
break;

case TM1638liteResetCmd: {
  if (validTM1638(unit) and (i == 0)) {
    TM1638s[unit]->reset();
  }
}
break;

case TM1638liteReadButtonsCmd: {
  if (validTM1638(unit) and (i == 0)) {
    bufferOut->write(TM1638s[unit]->readButtons());
  }
}
break;

case TM1638liteSetLEDCmd: {
  if (validTM1638(unit) and (i == 2)) {
    uint8_t position; uint8_t value;
    bufferIn->read(position);
    bufferIn->read(value);
    TM1638s[unit]->setLED(position, value);
  }
}
break;


// TM1638liteDisplayTextCmd is missing by design. It's implemented locally in 
// the controller library and uses TM1638liteDisplayASCII()


case TM1638liteDisplaySSCmd: {
  if (validTM1638(unit) and (i == 2)) {
    uint8_t position; uint8_t value;
    bufferIn->read(position);
    bufferIn->read(value);
    TM1638s[unit]->displaySS(position, value);
  }
}
break;

case TM1638liteDisplayASCIICmd: {
  if (validTM1638(unit) and (i == 2)) {
    uint8_t position; uint8_t ascii;
    bufferIn->read(position);
    bufferIn->read(ascii);
    TM1638s[unit]->displayASCII(position, ascii);
  }
}
break;

case TM1638liteDisplayHexCmd: {
  if (validTM1638(unit) and (i == 2)) {
    uint8_t position; uint8_t hex;
    bufferIn->read(position);
    bufferIn->read(hex);
    TM1638s[unit]->displayHex(position, hex);
  }
}
break;

#endif // MF_STAGE_processMessage


/*
   (6) reset event

*/
#if MF_STAGE == MF_STAGE_reset

for (uint8_t i = 0; i < numTM1638s; i++) {
  TM1638s[i]->reset();  // send reset command to all connected TM1638s
  delete TM1638s[i];    // destroy object allocated earlier with new()
}
numTM1638s = 0; // and reset object counter

#endif // MF_STAGE_reset



/// @endcond
