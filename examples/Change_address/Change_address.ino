/*
  AccelStepperI2C - Change I2C address demo
  (c) juh 2022

  Test and demonstrate changing the I2C address permanently.
  Just I2C-connect any target device with the AccelStepperI2C firmware
  and flash this demo to the controller.

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

  Serial.println("\n\n\nAccelStepperI2C change I2C address demo\n\n");
  delay(me);

  Serial.println("Trying to reach target at old address...");
  delay(me);

  if (wrapper.ping()) { // is target present at old oldAddress?

    Serial.print("Target found at old address "); Serial.println(oldAddress);
    delay(me);

    // new in v0.3.0
    Serial.print("I2C delay set to ");
    Serial.print(wrapper.autoAdjustI2Cdelay()); // uses startWith = 50 and defaults for safetyMargin of 2ms and max. buffer usage
    Serial.print(" ms (instead of default ");
    Serial.print(I2CdefaultDelay);
    Serial.println(" ms)");
    delay(1000);

    Serial.print("\nChanging address to "); Serial.println(newAddress);
    wrapper.changeI2Caddress(newAddress);
    delay(me);

    Serial.println("\nAddress changed. Rebooting target....\n");
    wrapper.reset();
    delay(me);

    // From here on we'll need to use the wrapperNew object, as the wrapper object is still bound to the old address

    Serial.println("Trying to reach target at new address...\n");
    delay(me);

    if (wrapperNew.ping()) {

      Serial.print("Target successfully found at new address ");
      Serial.print(newAddress); Serial.println("!\n\n");
      delay(me);

      Serial.println("Changing back to old address and rebooting...\n");
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
    Serial.print(oldAddress); Serial.println(", please check your connections and reboot.");
  }
  Serial.println("\n\nFinished.");
}

void loop()
{
}
