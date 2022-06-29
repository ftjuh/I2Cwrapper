/*
  AccelStepperI2C - Change I2C address demo
  (c) juh 2022

  Test and demonstrate changing the I2C address permanently.
  Just I2C-connect any target device with the AccelStepperI2C firmware
  and flash this demo to the controller.

  Note that if sth. goes wrong or if you interrupt this sketch prematurely,
  you'll end up with a target using the wrong address. You'll need to switch
  oldAddress and newAddress, then, and interrupt it again to make the old 
  address stick.

*/


#include <Arduino.h>
#include <I2Cwrapper.h>
#include <Wire.h>

const uint8_t oldAddress = 0x08;
const uint8_t newAddress = 0x07;
const int me = 2500; // delay between steps

I2Cwrapper wrapper(oldAddress);
I2Cwrapper wrapperNew(newAddress);


void setup()
{

  Wire.begin();
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("\n\n\nI2Cwrapper change I2C address demo\n\n");
  delay(me);

  Serial.println("Trying to reach target at old address...");
  delay(me);

  if (wrapper.ping()) { // is target present at old oldAddress?

    Serial.print("Target found at old address "); Serial.println(oldAddress);
    delay(me);

    // new in v0.3.0
    Serial.print(wrapper.autoAdjustI2Cdelay());
    Serial.println(" ms I2C delay");
    delay(me);

    Serial.print("\nChanging address to "); Serial.println(newAddress);
    wrapper.changeI2Caddress(newAddress);
    delay(me);

    Serial.println("\nAddress changed. Resetting target....\n");
    wrapper.reset();
    delay(me);

    // From here on we'll need to use the wrapperNew object, as the wrapper object is still bound to the old address

    Serial.println("Trying to reach target at new address...\n");
    delay(me);

    if (wrapperNew.ping()) {

      Serial.print("Target successfully found at new address ");
      Serial.print(newAddress); Serial.println("!\n\n");
      delay(me);

      Serial.println("Changing back to old address and resetting target again...\n");
      wrapperNew.changeI2Caddress(oldAddress);
      delay(me);
      wrapperNew.reset();
      delay(me);

      // and back to using the old wrapper object, again.

      if (wrapper.ping()) {
        Serial.println("Successfully changed back to old address.");
      } else {
        Serial.println("Sorry, could not change back to old address.");
      }
    } else {
      Serial.print("Target *not* found at new address ");
      Serial.print(newAddress); Serial.println(", something went wrong");
    }
  } else {
    Serial.print("Target not found at old address ");
    Serial.print(oldAddress); Serial.println(", please check your connections and restart.");
  }
  Serial.println("\n\nFinished.");
}

void loop()
{
}
