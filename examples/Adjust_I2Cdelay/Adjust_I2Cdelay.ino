/*
   Adjust_I2Cdelay demo
   (c) juh 2022

  Tests the new (in v0.3.0) autoAdjustI2Cdelay() function.
  Needs only the plain firmware on the target's side.

  The test will call autoAdjustI2Cdelay() repeatedly, with maxLength decreasing from
  its maximum (I2CmaxBuf - 3) to 1, to test the influence of varying buffer usage.
  
  The safetyMargin is set to the default 2 ms. It is just a constant added after 
  running the test, so there is no sense in testing it's influence.

  You can try it with differently configured target devices (firmware compiled
  with/without debugging; different hardware platforms etc.), or vary the lenght
  of your I2C connections, size of pullups etc. to test their effect (I don't
  expect the latter to have one).

  On an Arduino Nano w/o serial debugging, all test will probably come out with 3 ms,
  showing that max buffer usage will not really make a difference, and that I2C
  transmissions alone don't really need an I2C delay larger than 1 ms (remember, we set 
  2 ms as safety margin). It's what the target is actually doing, what's creating the 
  need for the delay.
  
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
    Serial.println("Target not found! Check connections and restart.\n\nHALTING\n");
    while (true) {}
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }
  wrapper.reset(); // reset the target device

}


void loop()
{
  Serial.println("\nTesting raw minimum delay with safetyMargin = 0 ms and decreasing values of maxLength\n");

  for (int i = I2CmaxBuf - 3; i > 0 ; i--) {
    Serial.print("maxLength = "); Serial.print(i); Serial.print(" --> I2C delay = ");
    for (int j = 0; j < 10; j++) {
      // startWith = 5 makes things a bit faster; if serial debugging is enabled for the target, though, you will probably need a larger value than 5
      Serial.print(wrapper.autoAdjustI2Cdelay(/* maxLenght */ i, /* safetyMargin */ 2, /* startWith */ 5));
      Serial.print(", "); 
      delay(50);
    }
    Serial.print("\n");
    delay(250);
  }
  delay(10000);
}
