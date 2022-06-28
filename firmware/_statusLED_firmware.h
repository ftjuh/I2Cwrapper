/*!
   @file _statusLED_firmware.h
   @brief Feature module that makes the LED_BUILTIN flash on incoming interrupts
   (receiveEvent and requestEvent). Meant mainly as a still-alive monitor.
   To make it flash on I2C state machine state changes, (un)comment the respective lines below in secitions (7), (8) and (9)
   If your board has no LED_BUILTIN, make sure to edit the proper pin number in the declaration stage below..
   @section author Author
   Copyright (c) 2022 juh
   @section license License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/

/// @cond

/*
   (1) includes
*/

#if MF_STAGE == MF_STAGE_includes
#endif // MF_STAGE_includes


/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations

#ifdef LED_BUILTIN
const uint8_t statusLED = LED_BUILTIN;
#else
const uint8_t statusLED = 9; // If your board has no LED_BUILTIN, change this to a pin with an LED attached, yourself.
#endif
const unsigned long statusFlashLength = 500; // microseconds
unsigned long startOfStatusFlash;
bool statusFlashIsOn = false;

void doTheStatusFlash(uint8_t pin) {
  digitalWrite(pin, HIGH);
  statusFlashIsOn = true;
  startOfStatusFlash = micros();
}

#endif // MF_STAGE_declarations


/*
   (3) setup() function
*/

#if MF_STAGE == MF_STAGE_setup
log("statusLED feature enabled.\n");
pinMode(statusLED, OUTPUT);
digitalWrite(statusLED, LOW);
#endif // MF_STAGE_setup


/*
   (4) main loop() function
*/

#if MF_STAGE == MF_STAGE_loop
if (statusFlashIsOn and (micros() - startOfStatusFlash) > statusFlashLength) {
  digitalWrite(statusLED, LOW);
  statusFlashIsOn = false;
}
#endif // MF_STAGE_loop


/*
   (5) processMessage() function
*/

#if MF_STAGE == MF_STAGE_processMessage
// @todo command to switch it on and off?
#endif // MF_STAGE_processMessage


/*
   (6) reset event
*/

#if MF_STAGE == MF_STAGE_reset
// there are no ressources to free, but make sure that the LED doesn't stay on
digitalWrite(statusLED, LOW);
statusFlashIsOn = false;
#endif // MF_STAGE_reset


/*
   (7) (end of) receiveEvent()
*/

#if MF_STAGE == MF_STAGE_receiveEvent
doTheStatusFlash(statusLED);
#endif // MF_STAGE_receiveEvent


/*
   (8) (end of) requestEvent()
*/

#if MF_STAGE == MF_STAGE_requestEvent
doTheStatusFlash(statusLED);
#endif // MF_STAGE_requestEvent


/*
   (9) Change of I2C state machine's state
*/

#if MF_STAGE == MF_STAGE_I2CstateChange
// doTheStatusFlash(statusLED); // uncomment to flash on I2C state change (better comment out above in sections (7) and (8), then
#endif // MF_STAGE_I2CstateChange



/// @endcond
