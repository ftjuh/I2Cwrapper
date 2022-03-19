# Introduction

This is an I2C wrapper for Mike McCauley's [AccelStepper library](https://www.airspayce.com/mikem/arduino/AccelStepper/index.html) with support for two end stops per stepper and optional servo and pin-expander support. It consists of an Arduino-based **firmware** for one or more I2C-slaves, and corresponding Arduino **libraries** for the I2C-master. Think of it as a more accessible and more flexible replacement for dedicated I2C stepper motor controller ICs like AMIS-30622, PCA9629 or TMC223 with some extra bells and whistles. Use it with your own hardware or with a plain stepper driver shield like the Protoneer CNC GRBL shield (recent [V3.51](https://www.elecrow.com/arduino-cnc-shield-v3-51-grbl-v0-9-compatible-uses-pololu-drivers.html) or [V3.00 clone](https://forum.protoneer.co.nz/viewforum.php?f=17)).

The firmware has been tested on ATmega328 Arduino (Uno, Nano etc.), ESP8266 and ESP32 platforms. I expect it should work on other Arduino supported platforms.

[Download AccelStepperI2C on github.](https://github.com/ftjuh/AccelStepperI2C)

[AccelStepperI2C library documentation](https://ftjuh.github.io/AccelStepperI2C/)

# How does it work?

* One or more Arduino-likes, acting as **I2C-slaves**, drive up to eight stepper motors each. The slaves run the AccelStepperI2C firmware ([firmware.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/firmware/firmware.ino)), which exposes the AccelStepper functionality via I2C, and additionally supports up to two end stop switches per stepper.
* Another device, acting as **I2C-master**, controls the slave(s) via I2C with the help of the AccelStepperI2C library. [Its interface](https://ftjuh.github.io/AccelStepperI2C/class_accel_stepper_i2_c.html)  is largely identical to the original AccelStepper library.
* If **servo support** is enabled at compile time, unused slave pins can be used to attach servos which the master controls with help of the ServoI2C library. [Its interface](https://ftjuh.github.io/AccelStepperI2C/class_servo_i2_c.html) is largely identical to the original Servo library.
* If **pin control support** is enabled at compile time, unused digital and analog slave pins can be used as   remote inputs or outputs with help of the PinI2C library, similar to I2C port expanders. [Its interface](https://ftjuh.github.io/AccelStepperI2C/class_pin_i2_c.html) is largely identical to the original Arduino pin control functions like digitalRead() or analogWrite().
* Internally, the slave is represented by an object of class **I2Cwrapper**. AccelStepperI2C, ServoI2C and PinI2C objects use an I2Cwrapper object for communication with the slave. All general communication with the slave (e.g. reset or error handling) is managed by the I2Cwrapper object, while all communication with a device unit (stepper or servo) is managed by an AccelStepperI2C or ServoI2C object.

Refer to the [example below](#example) and the [examples folder](https://github.com/ftjuh/AccelStepperI2C/tree/master/examples) to see how it works in detail.

# Differences from AccelStepper

## State machine vs. polling

The original AccelStepper needs the client to **constantly 'poll'** each stepper by invoking one of the run() commands (run(), runSpeed(), or runSpeedToPosition()) at a frequency which mustn't be lower than the stepping frequency. Over I2C, this would clutter the bus, put limits on stepper speeds, and be unstable if there are other I2C devices on the bus, particularly with multiple steppers and microstepping.

To solve this problem, the AccelStepperI2C firmware implements a __state machine__ for each connected stepper which makes the slave do the polling locally on its own.

All the master has to do is make the appropriate settings, e.g. set a target with AccelStepperI2C::moveTo() or choose a speed with AccelStepperI2C::setSpeed() and then start the slave's state machine (see example below) with one of 
+ AccelStepperI2C::runState(): will poll run(), i.e. run to the target with acceleration, and stop 
  the state machine upon reaching it
+ AccelStepperI2C::runSpeedState(): will poll runSpeed(), i.e. run at constant speed until told otherwise (see AccelStepperI2C::stopState()), or
+ AccelStepperI2C::runSpeedToPositionState(): will poll runSpeedToPosition(), i.e. run at constant speed until the target has been reached.

AccelStepperI2C::stopState() will stop any of the above states, i.e. stop polling. It does nothing else, so the master is solely in command of target, speed, and other settings.

## Additional features

   - Up to two **end stop switches** can be defined for each stepper. If enabled and the stepper runs into one of them, it will make the state machine (and the stepper motor) stop. Of course, this is most useful in combination with AccelStepperI2C::runSpeedState() for homing and calibration tasks at startup. See [Interrupt_Endstop.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/examples/Interrupt_Endstop/Interrupt_Endstop.ino) example for a use case.
   - An **interrupt pin** can be defined which informs the master that the state machine's state has changed. Currently, this will happen when a set target has been reached or when an endstop switch was triggered. See [Interrupt_Endstop.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/examples/Interrupt_Endstop/Interrupt_Endstop.ino) example for a use case.
   - The **slave's I2C address** (default: 0x8) can be permanently changed without recompiling the firmware.
   - I2C transmission **error checking** is available and recommended in critical situations (see [Error handling](#error-handling)).
   - **Speed diagnostics** are available (see [Performance and diagnostics](#performance-and-diagnostics)).
   - **Servo support** is available (see above and the [Servo_sweep.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/examples/Servo_Sweep/Servo_Sweep.ino) and [Stepper_and_Servo_together.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/examples/Stepper_and_Servo_together/Stepper_and_Servo_together.ino) examples). Can be disabled at compile time.
   - (New in v0.1.1) **Pin control support** is available (see above and the [Pin_control.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/examples/Pin_control/Pin_control.ino) example). Can be disabled at compile time.
   - The project has grown into a **framework** which could easily be extended to provide other additional slave capabilities like driving DC motors ~~or using remote pins (I/O extender like)~~ in the future.

## Restrictions

   - The original run(), runSpeed(), or runSpeedToPosition() functions are implemented, but it is **discouraged to use them directly**. The idea of these functions is that they are called as often as possible which would cause constant I2C traffic. The I2C protocol was not designed for this kind of traffic, so use the state machine instead. If you feel you *must* use the original ones, take it slowly and see if your setup, taking other I2C devices into consideration, allows you to [increase the I2C bus frequency](https://www.arduino.cc/en/Reference/WireSetClock). Even then you shouldn't poll as often as possible (as AccelStepper usually wants you to), but adjust the polling frequency to you max. stepping frequency, to allow the I2C bus some room to breathe.
   - Each library function call, its parameters, and possibly a return value has to be transmitted back and forth via I2C. This makes things **slower, less precise, and prone to errors**. To be safe from errors, you'll need to do some extra checking (see [Error handling](#error-handling)).
   - No **serialization protocol** is used at the moment, so the implementation is machine dependent in regard to the endians and sizes of data types. Arduinos Uno/Nano, ESP8266 and ESP32 have been tested and work well together, as should  other Arduino supporting platforms.
   - Naturally, you cannot declare your own stepping functions with the [constructor [2/2] variant](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#afa3061ce813303a8f2fa206ee8d012bd).
   - ~~The two blocking functions, **runToPosition() and runToNewPosition()**, are not implemented at the moment. I never saw their purpose, anyway, as their behavior can easily implemented with the existing functionality.~~ 
   - ~~There is no interrupt mechanism at the moment which tells the master that a state machine job has finished or an endstop has been triggered. So the master still needs to poll the slave with one of AccelStepperI2C::distanceToGo(), AccelStepperI2C::currentPosition, AccelStepperI2C::isRunning(), or AccelStepperI2C::endstops() but can do so at a much more reasonable frequency.~~
   - ~~Currently, there is no error checking of the master-slave communication, which uses a very basic (i.e. non existing) protocol. The master will have to deal with any transmission errors should they occur.~~

Have a look at the [todo list](https://ftjuh.github.io/AccelStepperI2C/todo.html) to see what improvements are still planned.

# Usage

A warning up front: Uncontrollably moving stepper motors can break things. So take care of **[error handling](#error-handling)** and **[safety precautions](#safety-precautions)** in critical environments e.g. if your steppers gone wild might damage something.

1. Install the folder with the libraries AccelStepperI2C, SimpleBuffer, I2Cwrapper (ServoI2C and PinI2C, if used) and of course the original AccelStepper [to the Arduino environment](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#manual-installation).
2. Upload [firmware.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/firmware/firmware.ino) to one Arduino (slave), there are a couple of configuration options at the top of the file. Connect steppers and stepper drivers to the slave or use a dedicated hardware, e.g. Arduino UNO with CNC V3.00 shield.
3. Upload an example sketch or your own master sketch to another Arduino-like (master).
4. Connect the I2C bus of both devices, usually it's A4<->A4 (SDA), A5<->A5 (SCL) and GND<->GND. Don't fortget two I2C pullups and, if needed, level-shifters. Also connect +5V<->+5V to power one board from the other, if needed.
5. Now (not earlier) provide external power to the steppers and power to the Arduinos.

Have a look at the [examples](https://github.com/ftjuh/AccelStepperI2C/tree/master/examples) for details. 

## Error handling

If I2C transmission problems occur, any call to the library could fail and, possibly worse, every return value could be corrupted. Uncontrolled or unexpected stepper movements could lead to serious problems (see below). That's why each command and response is sent with a CRC8 checksum. To find out if a master's command or a slave's response was **transmitted correctly**, the master can check the following: 

- If I2Cwrapper::sentOK is false, the previous function call was not properly transmitted. 
- If I2Cwrapper::resultOK is false, the data returned from the previous function call is invalid.

The library keeps an internal count of the **number of failed transmissions**, i.e. the number of cases that sentOK and resultOK came back false. If the master doesn't want to check each transmission separately, it can use one of the following methods at the end of a sequence of transmissions (e.g. setup and configuration of the slave or at the end of some program loop):

* uint16_t I2Cwrapper::sentErrors() - number of false sentOK events
* uint16_t I2Cwrapper::resultErrors() - number of false resultOK events
* uint16_t I2Cwrapper::transmissionErrors() - sum of the above

The respective counter(s) will be reset to 0 with each invocation of these methods.

See the [Error_checking.ino](https://github.com/ftjuh/AccelStepperI2C/blob/master/examples/Error_checking/Error_checking.ino) example for further illustration.

## Safety precautions

Steppers can exert damaging forces, even if they are moving slow. If in doubt, set up your system in a way that errors will not break things, particularly during testing:

* Place your end stops in a **non-blocking position** so that they are activated in a by-passing way but do not block the way themselves.
* To be really safe, put **emergency stops** which shut down the slave in a final end position. Currently there is no dedicated pin mechanism for that, so just use the slave's reset pin instead.
* Always keep a **manual emergency stop** at hand or make it easy to cut the power quickly.
* And again, do check for transmission errors in critical situations (see **[error handling](#error-handling)**).

## Troubleshooting

If things don't work as expected, here's a couple of things that helped me during testing:

* Start **without I2C**. That is, after setting up the hardware, first test the to-be-slave device with a conventional AccelStepper sketch, so that you can be sure your problems are I2C-related.
* Start **without steppers**. Use error handling and possibly debug output to test if communication between master and slave is working as it should.
* **Simplify**. Start with very simple setups, short cables, low bus speeds etc. and work your way up from there. Don't mix platforms, start with two Uno/Nanos.
* Did you think of two **I2C-pullups**?
* Have you powered your **level shifters**? They won't work with SCL, SDA and GND alone.
* If master and slave run on different hardware platforms, **pin names** like "LED_BUILTIN" or D3" might refer to completely different hardware pins or not be defined at all.  For example, when using a ESP8266 Wemos d1 mini as slave, a master sketch for the Arduino Uno cannot use "D3" as it is undefined, and, even worse, a master sketch compiled for some ESP32 device will translate it to a completely different hardware pin.  So when addressing slave pins from the master's side, it's safest to use the integer equivalents of names like "D3". Look them up in the `pins_arduino.h` file for your slave device or run a simple sketch with `Serial.print(D3);` etc. on your slave board.
* ESPs can crash if you unintentionally use certain pin numbers (e.g. flash memory pins) that are available on plain Unos/Nanos. Remember that when you **move between platforms**.
* Enable **debug output** on slave *and* master. It's a bit cluttered at the moment, yet informative. To see both master and slave output simultaneously, you need to open slave and master sketch from two independently started Arduino instances, i.e. don't use the open dialogue from your master's sketch to open the firmware sketch, instead start the program a second time. Only then you can open two separate serial output windows after choosing the two USB ports.

# Performance and diagnostics

On a normal ATmega328 @ 16MHz, the original AccelStepper supposedly can run [about 4000 steps per second](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#details). The AccelStepperI2C state machine only takes a tiny bite away from that. However, if you are using a lot of steppers, need high speeds, and/or if you are using a big microstepping factor, a normal Arduino can quickly become too slow. So choose your platform wisely, the ESP32 will be 10-20 times faster.

DEBUG output, which can be enabled for all parts of this software individually, will report a simple cycles/second statistic to Serial for the slave's state machine. This, however, will be distorted by the time needed for serial output.

If the slave firmware was compiled with the @ref DIAGNOSTICS compiler directive, more elaborate and more reliable speed measurements are available. You can use the diagnostics() function to investigate the system performance. It will use a struct of type @ref diagnosticsReport with information on

* the time needed to process (interpret) the most recently received command,
* the time the slave spent in the most recent onRequest() interrupt, 
* the time the slave spent in the most recent onReceive() interrupt, and
* the number of main loop executions since the last reboot. You can use this number to determine the frequency with which active state machines poll the AccelStepper library. This frequency always needs to be higher than the stepping frequency. Be warned that it is heavily influenced by the incidence of speed recalculations that the library does during acceleration and deceleration. Any change in one of the relevant parameters, e.g. acceleration factor or distance to target, will influence this incidence. So you'll need to determine the polling frequency under your specific real world conditions, not under some dry run testing conditions.

# Supported platforms

The following platforms will run the slave firmware and have been (more or less) tested. Unfortunately, they all have their pros and cons:

* **Arduino AVRs (Uno, Nano etc.)**: Comes with I2C hardware support which should make communication most reliable and allows driving the I2C bus at higher frequencies. With only 16MHz CPU speed not recommended for high performance situations with many steppers, microstepping and high speeds.
* **ESP8266**: Has no I2C  hardware. The software I2C will not work stable at the default 80MHz CPU speed, make sure to configure the **CPU clock speed to 160MHz**. Even then, it might be necessary to [decrease the bus speed](https://www.arduino.cc/en/Reference/WireSetClock) below 100kHz for stable bus performance, start as low as 10kHz if in doubt. Apart from that, expect a performance increase of ca. 10-15x vs. plain Arduinos due to higher CPU clock speed and better hardware support for math calculations.
* **ESP32**: Has no I2C  hardware. I2C is stable at the default 240MHz, but officially cannot run faster than 100kHz. Also, the slave implementation is awkward. It might be more susceptible for I2C transmission errors, so timing is critical (see AccelStepperI2C::setI2Cdelay()). Apart from that, expect a performance increase of ca. 15-20x vs. plain Arduinos due to higher CPU clock speed and better hardware support for math calculations.

# Example

```c++
/*
   AccelStepperI2C Bounce demo
   (c) juh 2022
   
   This is a 1:1 equivalent of the AccelStepper Bounce.pde example
   https://www.airspayce.com/mikem/arduino/AccelStepper/Bounce_8pde-example.html
*/

#include <Wire.h>
#include <AccelStepperI2C.h>

uint8_t i2cAddress = 0x08;

I2Cwrapper wrapper(i2cAddress); // each slave device is represented by a wrapper...
AccelStepperI2C stepper(&wrapper); // ...that the stepper needs to communicate with it

void setup()
{  
  Wire.begin();
  // Wire.setClock(10000); // uncomment for ESP8266 slaves, to be on the safe side

  if (!wrapper.ping()) {
    Serial.println("Slave not found! Check connections and restart.");
    while (true) {}
  }  
  wrapper.reset(); // reset the slave device
  delay(500); // and give it time to reboot
  
  stepper.attach(); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
  // attach() replaces the AccelStepper constructor, so it could also be called like this: 
  // stepper.attach(AccelStepper::DRIVER, 5, 6);
  
  if (stepper.myNum < 0) { // stepper could not be allocated (should not happen after a reset)
    while (true) {}
  }
    
  // Change these to suit your stepper if you want
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(100);
  stepper.moveTo(2000);
}

/* This is the recommended AccelStepperI2C implementation using the state machine.
 * Note that the polling frequency is not critical, as the state machine will stop 
 * on its own. So even if stepper.distanceToGo() causes some I2C traffic, it will be 
 * substantially less traffic than sending each stepper step seperately (see below).
 * If you want to cut down I2C polling completely, you can use the interrupt mechanism 
 * (see Interrupt_Endstop.ino example sketch).
 */
void loop()
{
  stepper.runState(); // start the state machine with the set target and parameters
  while (stepper.distanceToGo() != 0) { // wait until target has been reached
    delay(250); // just to demonstrate that polling frequency is not critical
  }
  stepper.moveTo(-stepper.currentPosition());   
}

/* This is the "classic" implementation which uses the original polling functions. 
 * It will work, at least in low performance situations, but will clog the I2C bus as 
 * each (attempted) single stepper step needs to be sent via I2C.
 */
void loopClassic()
{
  if (stepper.distanceToGo() == 0)
    stepper.moveTo(-stepper.currentPosition());
  stepper.run(); // frequency is critical, each call will cause I2C traffic
}
```

# Documentation

[Find the AccelStepperI2C library documentation here](https://ftjuh.github.io/AccelStepperI2C/). It only deals with differences from running the AccelStepper library locally. If not stated otherwise, expect [functions and parameters](@ref AccelStepperI2C) to work as in the original, keeping the above differences and restrictions in mind.

# Author

This is my first "serious" piece of software published on github. Although I've some background in programming, mostly in the Wirth-tradition languages, I'm far from being a competent or even avid c++ programmer. At the same time I have a tendency to over-engineer (not a good combination), so be warned and use this at your own risk. My current main interest is in 3D printing, you can find me on [prusaprinters](https://www.prusaprinters.org/social/202816-juh/about), [thingiverse](https://www.thingiverse.com/juh/designs), and [youmagine](https://www.youmagine.com/juh3d/designs). This library was developed as part of my [StepFish project](https://www.prusaprinters.org/prints/115049-stepfish-fischertechnik-i2c-stepper-motor-controll) ([also here](https://forum.ftcommunity.de/viewtopic.php?t=5341)).

Contact me at ftjuh@posteo.net.

Jan (juh)

# Copyright

This software is Copyright (C) 2022 juh (ftjuh@posteo.net)

# License

AccelStepperI2C is distributed under the GNU GENERAL PUBLIC LICENSE Version 2.

# History

v0.1.0 Initial release

v0.1.1 Fixed I2Cdelay; 

v0.2.0 Added pin control support (PinI2C.h and PinI2C.cpp) for reading and writing unused slave pins;     Restructured folder layout to prepare for submission as third party library to the Arduino library manager.

v0.2.1 updated library.properties