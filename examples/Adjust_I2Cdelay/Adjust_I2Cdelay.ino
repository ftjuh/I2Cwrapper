/*
   Adjust_I2Cdelay demo
   (c) juh 2022

  Tests the new (in v0.3.0) autoAdjustI2Cdelay() function.
  Needs only the plain firmware on the target's side.

  The test will call autoAdjustI2Cdelay() with the safetyMargin set to 0 ms. It is just a
  constant added after running the test, so there is no sense in testing it's
  influence. maxLength is


  You can try it with differently configured target devices (firmware compiled
  with/without debugging; different hardware platforms etc.), or vary the lenght
  of your I2C connections, size of pullups etc. to see their effect.
*/


#include <Wire.h>
#include <I2Cwrapper.h>

uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress);


void setup()
{
  Wire.begin();
  Serial.begin(115200);
  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side

  if (!wrapper.ping()) {
    halt("Target not found! Check connections and restart.");
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }
  wrapper.reset(); // reset the target device
  delay(500); // and give it time to reboot

}


void loop()
{
  Serial.println("\nTesting raw minimum delay with safetyMargin = 0 ms and increasing values of maxLength\n");

  for (int i = /*I2CmaxBuf - 3*/ 4; i > 0 ; i--) {
    Serial.print("maxLength = "); Serial.print(i); Serial.print(" --> I2C delay = ");
    for (int j = 0; j < 10; j++) {
      Serial.print(wrapper.autoAdjustI2Cdelay(0, i)); Serial.print(", ");
      delay(50);
    }
    Serial.print("\n");
    delay(250);
  }
  delay(10000);
}

void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}
