/*
 * TM1638liteI2C demo
 * 
 * Based on the TM1638lite library's original demo.ino
 * by Danny Ayers (https://github.com/danja),
 * adapted for I2C by juh 2022
 * 
 * Therefore, like TM1638lite, distributed under the
 * Apache License Version 2.0, January 2004 http://www.apache.org/licenses/
 * 
 * Apart from a different setup, this is the original TM1638lite
 * demo.ino by Danny Ayers adapted for running over I2C with the
 * the I2Cwrapper firmware with TM1638liteI2C module enabled.
 * 
 * Connect a "LED & Key" module to the target device's pins given below,
 * upload the firmware with TM1638liteI2C module to the target device and
 * this sketch to the controller device, both connected by I2C. Expect the
 * overall speed to be a bit more laggy than normal, adjust the I2C
 * delay to improve it (see comment below in setup() function).
 * 
 * Tested with the firmware running on Arduino Nano and ATtiny85 (with ATTinyCore, 
 * no bootloader). Note that the TM1638 device works with 5V, too.
 * 
 */

#include <Wire.h>
#include <TM1638liteI2C.h>

const uint8_t i2cAddress = 0x08;


// Pins for plain Arduinos, taken from Danny's demo.ino. Any other pins should work, too.
const uint8_t strobePin = 4;
const uint8_t clockPin  = 7;
const uint8_t dataPin   = 8;

// Pins for ATtiny85 (I2C is on pins 0/SDA and 2/SCL)
// (Watch out, these pin numbers are not the physical pins but the "Arduino Pins" given at
// https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md )
// const uint8_t strobePin = 3; // physical pin 2/PB3
// const uint8_t clockPin  = 4; // physical pin 3/PB4
// const uint8_t dataPin   = 1; // physical pin 6/PB1


I2Cwrapper wrapper(i2cAddress); // each target device is represented by a wrapper...
TM1638liteI2C tm(&wrapper); // ...that the TM1638 object needs to communicate with the controller
 
// this section from demo.ino commented out, see attach() function call in setup()
// I/O pins on the Arduino connected to strobe, clock, data
// (power should go to 3.3v and GND)
// TM1638liteI2C tm(5, 6, 7);


void setup() {
  
  Wire.begin();
  Serial.begin(115200);
  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side
  
  if (!wrapper.ping()) {
    halt("Target not found! Check connections and restart.");
  } else {
    Serial.println("Target found as expected. Proceeding.\n");
  }
  wrapper.reset(); // reset the target device
  
  /*
   *    You'll notice a tiny delay in execution vs. the original demo, e.g.
   *    during text display. This is a side effect of the I2C transmissions, or,
   *    more precisely, the safety delay the I2C controller device keeps
   *    between two transmissions to make sure that the target device is not
   *    flooded more quickly than it can react. The default of 20 ms usually can
   *    safely be decreased, if debugging is not enabled at the target's side.
   *    To do so, uncomment the following line and lower the value until errors
   *    start to occur. I could go as low as 4 ms, making the delay
   *    nearly unnoticable (on Arduino and ATtiny85@8MHz internal oscillator).
   */
  // wrapper.setI2Cdelay(8);
  
  tm.attach(strobePin, clockPin, dataPin); // this replaced the constructor above
  if (tm.myNum < 0) {
    halt("Could not attach device. Halting.\n\n");
  }
  
  
  /*
   *    From here on, the code is unmodified from Danny's original demo.ino.
   */
  
  tm.reset();
  
  tm.displayText("Eh");
  tm.setLED(0, 1);
  
  delay(2000);
  
  tm.displayASCII(6, 'u');
  tm.displayASCII(7, 'p');
  tm.setLED(7, 1);
  
  delay(2000);
  
  tm.displayHex(0, 8);
  tm.displayHex(1, 9);
  tm.displayHex(2, 10);
  tm.displayHex(3, 11);
  tm.displayHex(4, 12);
  tm.displayHex(5, 13);
  tm.displayHex(6, 14);
  tm.displayHex(7, 15);
  
  delay(2000);
  
  tm.displayText("buttons");
}

void loop() {
  uint8_t buttons = tm.readButtons();
  doLEDs(buttons);
}

// scans the individual bits of value
void doLEDs(uint8_t value) {
  for (uint8_t position = 0; position < 8; position++) {
    tm.setLED(position, value & 1);
    value = value >> 1;
  }
}


void halt(const char* m) {
  Serial.println(m);
  Serial.println("\n\nHalting.\n");
  while (true) {
    yield(); // prevent ESPs from watchdog reset
  }
}
