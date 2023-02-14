/*
   ServoI2C Sweep demo
   (c) juh 2022

   This is a 1:1 equivalent of the Servo library's Sweep example
   https://docs.arduino.cc/learn/electronics/servo-motors

   Needs the ServoI2C.h module enabled in the target's firmware_modules.h.
   
*/

#include <Wire.h>
#include <ServoI2C.h>

uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress); // each target device is represented by a wrapper...
ServoI2C myservo(&wrapper); // ...that the servo needs to communicate with the controller

int pos = 0;    // variable to store the servo position

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
  
  myservo.attach(9); // attaches the servo on _the target's_ pin "9" to the servo object

}

void loop()
{
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}
