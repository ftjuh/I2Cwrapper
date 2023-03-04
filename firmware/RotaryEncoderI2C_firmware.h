/*!
   @file RotaryEncoderI2C_firmware.h
   @brief

   ## Author
   Copyright (c) 2023 juh
   ## License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/

/// @cond

/*
   (1) includes

*/
#if MF_STAGE == MF_STAGE_includes
#include <RotaryEncoderI2C.h>
#endif // MF_STAGE_includes


/*
   (2) declarations

*/
#if MF_STAGE == MF_STAGE_declarations

bool diagnosticsMode = false;
uint8_t diagnosticsEncoder = 0; // the encoder to diagnose

#if defined(ARDUINO_AVR_ATTINYX5) // ### include all other tinys
const uint8_t maxRotaryEncoders = 2; // limited memory and pins
#else
const uint8_t maxRotaryEncoders = 8;
#endif
uint8_t numRotaryEncoders = 0; // number of initialised encoders

//RotaryEncoder* encoders[maxRotaryEncoders];
//uint8_t encoderPins[maxRotaryEncoders][2];

struct AttachedEncoder {
  RotaryEncoder* encoder;
  uint8_t pin1;
  uint8_t pin2;
};
AttachedEncoder encoders[maxRotaryEncoders];


bool validEncoder(int8_t u)
{
  return (u >= 0) and (u < numRotaryEncoders);
}



#endif // MF_STAGE_declarations


/*
   (3) setup() function

*/
#if MF_STAGE == MF_STAGE_setup
log("RotaryEncoderI2C module enabled.\n");
#endif // MF_STAGE_setup


/*
   (4) main loop() function

*/
#if MF_STAGE == MF_STAGE_loop

if (not diagnosticsMode) {
  for (uint8_t enc = 0; enc < numRotaryEncoders; enc++) {
    encoders[enc].encoder->tick();
  }
}

#endif // MF_STAGE_loop


/*
   (5) processMessage() function

*/
#if MF_STAGE == MF_STAGE_processMessage

case rotaryEncoderAttachCmd : {
  if (i == 5) { // int pin1, int pin2, LatchMode mode
    int16_t pin1; bufferIn->read(pin1);
    int16_t pin2; bufferIn->read(pin2);
    uint8_t mode; bufferIn->read(mode);
    int8_t res;
    if (numRotaryEncoders < maxRotaryEncoders) {
      encoders[numRotaryEncoders].encoder = new RotaryEncoder((int)pin1, (int)pin2, (RotaryEncoder::LatchMode)mode);
      encoders[numRotaryEncoders].pin1 = uint8_t(pin1);
      encoders[numRotaryEncoders].pin2 = uint8_t(pin2);
      log("Add rotary encoder with internal myNum = "); log(numRotaryEncoders); log("\n");
      res = numRotaryEncoders++;
    } else {
      log("-- Too many encoders, failed to add new one\n");
      res = -1;
    }
    bufferOut->write(res);
  }
}
break;

case rotaryEncoderGetPositionCmd : {
  if ((i == 0) and validEncoder(unit)) {
    bufferOut->write((int32_t)encoders[unit].encoder->getPosition());
  }
}
break;

case rotaryEncoderGetDirectionCmd : {
  if ((i == 0) and validEncoder(unit)) {
    bufferOut->write((int8_t)encoders[unit].encoder->getDirection());
  }
}
break;

case rotaryEncoderSetPositionCmd : {
  if ((i == 4) and validEncoder(unit)) {
    uint32_t newPosition = 0; bufferIn->read(newPosition);
    encoders[unit].encoder->setPosition((long)newPosition);
  }
}
break;

case rotaryEncoderGetMillisBetweenRotationsCmd : {
  if ((i == 0) and validEncoder(unit)) {
    bufferOut->write((unsigned long)encoders[unit].encoder->getMillisBetweenRotations());
  }
}
break;

case rotaryEncoderGetRPMCmd : {
  if ((i == 0) and validEncoder(unit)) {
    bufferOut->write((unsigned long)encoders[unit].encoder->getRPM());
  }
}
break;

case rotaryEncoderStartDiagnosticsModeCmd : {
  if ((i == 1) and validEncoder(unit)) {
    bufferIn->read(diagnosticsEncoder); // global variable
    diagnosticsMode = true;
  }
}
break;


#endif // MF_STAGE_processMessage


/*
   (6) reset event

*/
#if MF_STAGE == MF_STAGE_reset

for (uint8_t enc = 0; enc < numRotaryEncoders; enc++) {
  delete encoders[enc].encoder;
  pinMode(encoders[enc].pin1, INPUT);
  pinMode(encoders[enc].pin2, INPUT);
}
numRotaryEncoders = 0;
diagnosticsMode = false;

#endif // MF_STAGE_reset



/*
   (8) requestEvent()

*/
#if MF_STAGE == MF_STAGE_requestEvent

if (diagnosticsMode) { 
  // this is a bit of a hack to circumvent having to have received a command before preparing a reply
  bufferOut->reset();
  bufferOut->write(uint8_t(uint8_t(digitalRead(encoders[diagnosticsEncoder].pin1)) | uint8_t((digitalRead(encoders[diagnosticsEncoder].pin2) << 1))));
  changeI2CstateTo(readyForResponse);
}


#endif // MF_STAGE_requestEvent


/// @endcond
