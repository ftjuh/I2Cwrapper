/*
   RotaryEncoderI2C demo, adapted from RotaryEncoder library's
   SimplePollRotator.ino example

   Assumes a segmented disc with n phases and two light barrier switches,
   shifted by a 1/4 phase against each other. Should work with other switches
   (Hall, mechanical, ...) as well, as long as both together generate a
   phase-shifted quadrature signal pattern.

   Pins configured for Quadrature Encoder for fischertechnik (https://www.thingiverse.com/thing:5867323)
   Change pins as needed. Note the comment on getDirection() and its altered behavior in I2Cwrapper context.

   Uncomment the below #define DIAGNOSTICS_MODE to test diagnostics mode.

   (c) juh 2023
   based on SimplePollRotator.ino example from RotaryEncoder library (c) by Matthias Hertel, http://www.mathertel.de

*/


#include <Wire.h>
#include <RotaryEncoderI2C.h>

// #define DIAGNOSTICS_MODE // uncomment this for diagnostics mode demo

#define PIN_IN1 4 // Attiny85 PB3, physical pin 3
#define PIN_IN2 1 // Attiny85 PB1, physical pin 6


uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress); // each I2Cwrapper target device is represented by a wrapper...
RotaryEncoderI2C encoder(&wrapper); // ...that the RotaryEncoder interface needs to communicate with the controller


void setup()
{

  Wire.begin();
  Serial.begin(115200);
  while (! Serial); delay(200);

  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side

  if (!wrapper.ping()) {
    halt("Target not found! Check connections and restart.");
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }
  wrapper.reset(); // reset the target device

  // this replaces the RotaryEncoder constructor and takes exactly the same arguments
  int8_t numEnc = encoder.attach(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);
  if (numEnc != -1) {
    Serial.print("Encoder attached as encoder #"); Serial.println(numEnc);
  } else {
    halt("Error: Too many encoders attached to target.");
  }

#ifdef DIAGNOSTICS_MODE
  wrapper.setI2Cdelay(8); // We probably want the signal to come in fast, so lower this as much as possibile
  encoder.startDiagnosticsMode(numEnc); // send pin states for this encoder; note there's no going back here, you'll have to reset the device to return to normal behavior
  loopDiagnostics(); // will never return
#endif // #ifdef DIAGNOSTICS_MODE

  Serial.println("SimplePollRotator example for I2CWrapper's RotaryEncoderI2C library.");

}


// Read the current position of the encoder and print it out when changed.
void loop()
{
  static int pos = 0;
  // encoder.tick(); // not needed (in fact, not implemented). The target will do that for us.

  int newPos = encoder.getPosition(); // get the current position from I2C target

  /* Note that if the target will execute tick() in its local loop (which is basically all
     it does apart from serving I2C requests) between the getPosition() call above and
     the getDirection() call in the line below, both results will mismatch. Direction could be
     NOROTATION (0), even if a new position was reported. That's why we moved the getDirection()
     up here, to reduce that probability, yet it cannot be fully prevented. Make your code robust
     against this possibility.
  */

  RotaryEncoder::Direction dir = encoder.getDirection();
  if (pos != newPos) {
    Serial.print("pos:"); Serial.print(newPos);
    Serial.print(" dir:"); Serial.print((int)dir);
    Serial.print(" ms:"); Serial.print(encoder.getMillisBetweenRotations());
    Serial.print(" rpm:"); Serial.println(encoder.getRPM());
    pos = newPos;

  }
  delay(20); // don't clog the I2C bus
}

void loopDiagnostics()
{
  while (true) {
    uint8_t pinstates = encoder.getDiagnostics();
    if (pinstates != 0xFF) {
      Serial.print(pinstates & 0x1); Serial.print(" ");  Serial.println((pinstates & 0x2) >> 1);
    } else {
      Serial.println("getDiagnostics() error");
    }
    //delay(10);
  }
}


void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}
