(⮤ Did you know that github hides a contents menu at the top left of this line?)

# Introduction

I2Cwrapper is a **generic modular framework for Arduino I2C target devices**. It supports AVR-based Arduinos (Uno, Nano etc.), ESP8266, and ESP32 platforms. 

I2Cwrapper consists of an easily extensible firmware framework and a controller library. Together, they **take care of the overhead** necessary for implementing an I2C target device and let you concentrate on implementing the actual target functionality. 

## Available modules

In the [firmware subfolder](firmware) (see below for more detailed information):

* **AccelStepperI2C**: Control up to eight stepper motors with acceleration control via Mike McCauley's [AccelStepper](https://www.airspayce.com/mikem/arduino/AccelStepper/index.html) library, and up to two end stops per stepper. Uses a state machine and an optional controller interrupt line to prevent I2C bus clogging. Works nearly like the original library. 
* **ServoI2C**: Control servo motors via I2C. Works just like the plain Arduino [`Servo`](https://www.arduino.cc/reference/en/libraries/servo) library.
* **PinI2C**: Control the digital and analog in- and output pins of the target device via I2C, similar to an IO-expander. Works just like as the plain Arduino digitalRead() etc. commands.
* **ESP32sensorsI2C**: Read an ESP32's touch sensors, hall sensor, and (if available) temperature sensor via I2C. Uses the optional controller interrupt line to inform the controller about a touch button press. Works nearly like the original ESP32 functions.

Modules can be selected in **any combination** at compile time for a specific target (see below for details). It is easy to **[add new modules](#how-to-add-new-modules)** with help of the provided **[templates](templates)**.

[Download I2Cwrapper on github.](https://github.com/ftjuh/I2Cwrapper)

[I2Cwrapper library documentation](https://ftjuh.github.io/I2Cwrapper/)

Note that I2Cwrapper uses the [current I2C terminology](https://www.nxp.com/docs/en/user-guide/UM10204.pdf) which replaced *master* with *controller*, and *slave* with *target*.

## Basic components

The I2Cwrapper framework consists of **four basic components.** The first two drive the **I2C target device**:

1. A **firmware framework** for the target device [(`firmware.ino`)](firmware/firmware.ino). It provides the basic I2C target functionality:
   - **onReceive**() and **onRequest**() interrupt functions that listen and react to the controller's transmissions,
   - a **command interpreter** that processes the controller's commands received by onReceive() (in traditional I2C hardware this is equivalent to register writes and reads),
   - an **output Buffer** that allows the target to prepare a reply which  will be sent upon the next onRequest() event,
   - transmission **error control** with CRC8-checksums, 
   - different ways for setting the target's **I2C address**: fixed address; EEPROM stored; and (not implemented yet) read from hardware pins,
   - a controller **interrupt** mechanism, 
   - triggering a **target reset**.
2. **Firmware modules** that implement the actual functionality of the target device, e.g. controlling stepper and/or servo motors.
   - Modules exist as separate **include files** (e.g. [`ServoI2C_firmware.h`](firmware/ServoI2C_firmware.h) ) and are **selected** for compilation via the [`firmware_modules.h`](firmware/firmware_modules.h) file.
   - Modules don't have to worry about the I2C overhead but can **concentrate on what's important**: interpreting and reacting to the controller device's commands and requests.
   - Modules can "inject" their code at different places in the firmware, so that there is a **high degree of flexibility**.

The other two basic components are for the **I2C controller's side**:

1. The [**I2Cwrapper class**](https://ftjuh.github.io/I2Cwrapper/class_i2_cwrapper.html), provided by the `I2Cwrapper.h` library. 
   - Controller sketches use an object of type I2Cwrapper to **represent the target** device which handles all low level communication tasks like CRC8 checksums, error handling etc.
   - It also provides **basic functions** for target device management like changing its I2C address, setting an interrupt pin, or making it reset.
2. **Controller libraries** for each module (e.g. [`ServoI2C.h`](src/ServoI2C.h)). 
   - controller libraries use **I2Cwrapper objects** to talk to the target device (like the class `ServoI2C` in `ServoI2C.h`).
   - They implement an interface for the respective target functionality, transmit each function call to the target device, and receive the target's reply, if the command was asking for it.

## Limitations

### Limitations for end users

   - Arduinos aren't perfect I2C target devices. Particularly, the Arduino's Wire library doesn't support [clock stretching](https://onlinedocs.microchip.com/pr/GUID-CA6B1F8F-E09D-4780-A52A-CBBF61902464-en-US-2/GUID-5CCAB0DB-28BD-4095-B2E2-2F3CF0FC6966.html) in a way that allows the target to finish reacting to the previous command if it hasn't done so yet before the transmission occurred. That's why it's important to make sure that the **target is not flooded** with commands or requests with too little time to handle them. I2Cwrapper provides an [adjustable minimum delay](#adjusting-the-i2c-delay) between transmissions to handle that problem.

### Limitations for module authors

   - No **serialization protocol** is used at the moment, so the implementation is machine dependent in regard to the endians and sizes of data types. Modules will have to take care that transmitted commands and requests will transmit defined amounts of bytes by using typeguards for ambiguously sized datatypes like *int*.
   - Modules use one byte **command codes** for each distinct function call to the target. At the moment, no mechanism is in place to prevent newly developed modules from reusing codes already used by another module or by one of the I2Cwrapper core functions. Note that this will only lead to problems if two modules are used concurrently by a target device. The [I2Cwrapper documentation](https://ftjuh.github.io/I2Cwrapper/docs/class_i2_cwrapper.html) has a list of code ranges used by the currently available modules. Strictly reserved ranges are 0-9 and 240-255.

See [below](#how-to-add-new-modules) if you are interested in writing a new module and implementing your own target device.

# Usage

## Installation

~~Install I2Cwrapper from the Arduino library manager (not available yet).~~ Until I2Cwrapper is picked up by the Arduino library manager, you'll have to [install it manually](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries#manual-installation) by copying the I2Cwrapper folder to your Arduino library folder and (re)starting the Arduino software.  You'll find an I2Cwrapper examples folder after successful installation.

## Target side: configuring and uploading the firmware

* **Open [firmware.ino](https://github.com/ftjuh/AccelStepperI2C/blob/controller/firmware/firmware.ino)**  from the examples menu of the Arduino editor, you'll find it in the the I2Cwrapper submenu. It will open multiple tabs, among them one for each available module in the firmware subfolder.
* Go to  the [`firmware_modules.h`](firmware/firmware_modules.h) tab and **select the modules** you want by (un)commenting them. For a first test, start with the `PinI2C` module, it is the simplest and doesn't need any extra hardware.
* You can **save a local copy** of the firmware. Don't forget, though, that your local copy won't be updated in future releases which might result in conflicts after a library upgrade.
* **Compile and upload** to your target device.

### Testing the firmware

The target device is now ready. To test it, you can use one of the example sketches:

* (optional, only tested for Linux) Open a completely **new instance** of the Arduino environment from your start menu. That way, you can connect target and controller devices at the same time without the need to change USB ports for uploading and serial output.
* Depending on the module(s) you selected, load **one of the examples** from the example folder and upload it to your controller device. Use `Pin_control.ino` for a first test with the `PinI2C` module.
* **Configure and upload** the example sketch or your own controller sketch to another Arduino-like (controller).
* **Connect the I2C bus** of both devices (including GND). Don't forget  I2C pullups and, and if needed, level-shifters. Also, connect V+ <-> V+ to power one board from the other, if needed.
* **Run** the controller sketch.

Have a look at the [examples](https://github.com/ftjuh/AccelStepperI2C/tree/controller/examples) for details. 

## Usage by the controller device/sketch

Simply include the **controller libraries** for the module(s) you compiled into your target firmware and use them as shown in the documentation and example sketches of the respective modules.

### Addressing target pins

Many functions take target pin numbers as an argument, e.g. when you define an interrupt pin with `I2Cwrapper::setInterruptPin()`. If controller and target devices run on **different hardware platforms**, you'll have to be careful that the controller addresses  the target's side pins correctly. Pin constants like `A0`, `D1`, `LED_BUILTIN` etc. might not be known at the controller's side or, even worse, might represent a different pin number. In this case it is recommended to use the raw pin numbers. They are defined in the respective platform's `pins_arduino.h` file, or can easily be found out by running `Serial.println(A0);` etc. on the target platform.

### Error handling

If I2C transmission problems occur, any command to the I2C target could fail and every return value could be corrupted. Depending on context, this could lead to severe consequences, e.g. with uncontrolled  stepper motor movements. That's why I2Cwrapper transmits each command and response with a CRC8 checksum. To find out if a controller's command or a target's response was **transmitted correctly**, the controller can check the following: 

- If `I2Cwrapper::sentOK` is false, the previous function call was not properly transmitted. 
- If `I2Cwrapper::resultOK` is false, the data returned from the previous function call is invalid.

The library keeps an internal count of the **number of failed transmissions**, i.e. the number of cases that sentOK and resultOK came back false. If the controller doesn't want to check each transmission separately, it can use one of the following methods at the end of a sequence of transmissions (e.g. after setup and configuration of the target, or at the end of some program loop):

* `uint16_t I2Cwrapper::sentErrors()` - number of false `sentOK` events
* `uint16_t I2Cwrapper::resultErrors()` - number of false `resultOK` events
* `uint16_t I2Cwrapper::transmissionErrors()` - sum of the above

The respective counter(s) will be reset to 0 with each invocation of these methods.

See the [Error_checking.ino](examples/Error_checking/Error_checking.ino) example for further illustration.

### Interrupt mechanism

To keep the controller from having to constantly poll the target device for some new event, the controller can use the `I2Cwrapper::setInterruptPin()` function to tell the target to use one if its pins as an **interrupt line**. The target may use it if it wants to inform the controller about some new event. Of course, an additional hardware line connecting this target pin and a free controller pin is needed to use the interrupt mechanism.

The **controller** will have to implement an interrupt service routine (ISR) to listen to the respective controller pin. After having received an interrupt, it must call `I2Cwrapper::clearInterrupt()` to clear the target's interrupt state and find out about the reason that caused the interrupt. 

Interrupt reasons are **specific for a module**. A module can send an interrupt to the controller with the `triggerInterrupt()` function which is provided by the `firmware.ino` framework. It can provide additional information on the interrupt reason and the target device's (sub)unit that caused the interrupt.

See the example [Interrupt_Endstop](examples/Interrupt_Endstop/Interrupt_Endstop.ino) for further illustration.

### Adjusting the I2C delay

If a controller sends commands too quickly or requests a target device's response too quickly after having sent a command, the target might not have finished processing the previous command and will not be ready to react appropriately. Usually, it should not take more than very few microseconds for the target to be ready again, yet particularly when serial debugging is enabled for the target it can take substantially longer. 

That's why I2Cwrapper makes sure that a **specified minimum delay** is kept between each transmission to the target, be it a new command or a request for a reply. The default minimum delay of 20 ms (in v3.0.0) is chosen deliberately conservative to have all  bases covered. Depending on debugging, target device speed, target task execution time, and bus speed, the default can be adjusted to be considerably lower with the I2Cwrapper::setI2Cdelay() function.

At the moment, you'll have to **use your own tests** to find an optimal value. A self-diagnosing auto-adjustment feature is planned for a future release.

# Available modules

To chose which modules are supported by an I2C target device, edit the [`firmware_modules.h`](firmware/firmware_modules.h) file accordingly.

## AccelStepperI2C

The AccelStepperI2C module provides access to **up to eight stepper motors** over I2C. It uses Mike McCauley's [AccelStepper library](https://www.airspayce.com/mikem/arduino/AccelStepper/index.html) and supports **two end stops per stepper**.  Think of it as a more accessible and more flexible replacement for dedicated I2C stepper motor controller ICs like AMIS-30622, PCA9629 or TMC223 with some extra bells and whistles. Use it with your own hardware or with a plain stepper driver shield like the Protoneer CNC GRBL shield (recent [V3.51](https://www.elecrow.com/arduino-cnc-shield-v3-51-grbl-v0-9-compatible-uses-pololu-drivers.html) or [V3.00 clone](https://forum.protoneer.co.nz/viewforum.php?f=17)).

### State machine

The original AccelStepper needs the client to **constantly 'poll'** each stepper by invoking one of the `run()` commands (`run()`, `runSpeed()`, or `runSpeedToPosition()`) at a frequency which mustn't be lower than the stepping frequency. Over I2C, this would clutter the bus, put limits on stepper speeds, and be unstable if there are other I2C devices on the bus, particularly with multiple steppers and microstepping.

To solve this problem,  AccelStepperI2C implements a __state machine__ in the target device's main loop for each connected stepper which makes the target do the polling locally on its own.

All the controller has to do is make the appropriate settings, e.g. set a target with `AccelStepperI2C::moveTo()` or choose a speed with `AccelStepperI2C::setSpeed()` and then start the target's state machine (see example below) with one of 

+ `AccelStepperI2C::runState()`: will poll `run()`, i.e. run to the target with acceleration, and stop 
  the state machine upon reaching it
+ `AccelStepperI2C::runSpeedState()`: will poll `runSpeed()`, i.e. run at constant speed until told otherwise (see `AccelStepperI2C::stopState()`), or
+ `AccelStepperI2C::runSpeedToPositionState()`: will poll `runSpeedToPosition()`, i.e. run at constant speed until the target has been reached.

`AccelStepperI2C::stopState()` will stop any of the above states, i.e. stop polling. It does nothing else, so the controller is solely in command of target, speed, and other settings.

### End stop switches

Up to **two end stop switches** can be defined for each stepper. If enabled and the stepper runs into one of them, it will make the state machine (and the stepper motor) stop.

Of course, this is most useful in combination with `AccelStepperI2C::runSpeedState()` for homing and calibration tasks at startup. See [`Interrupt_Endstop.ino`](examples/Interrupt_Endstop/Interrupt_Endstop.ino) example for a use case.

### Interrupt mechanism

I2Cwrapper's interrupt mechanism can be used to inform the controller that the state machine's state has changed. Currently, this will happen when a set **target has been reached** or when an **endstop** switch was triggered. See [`Interrupt_Endstop.ino`](examples/Interrupt_Endstop/Interrupt_Endstop.ino) example for a use case.

### Restrictions

   - The original `run()`, `runSpeed()`, or `runSpeedToPosition()` functions are implemented, but it is **not recommended to use them**. The idea of these functions is that they are called as often as possible which would cause constant I2C traffic. The I2C protocol was not designed for this kind of load, so use the state machine instead. If you feel you *must* use the original ones, take it slow and see if your setup, taking other I2C devices into consideration, allows you to [increase the I2C bus frequency](https://www.arduino.cc/en/Reference/WireSetClock). Even then you shouldn't poll as often as possible (as AccelStepper usually wants you to), but adjust the polling frequency to your max. stepping frequency, to allow the I2C bus some room to breathe.
   - Each library function call, its parameters, and possibly a return value has to be transmitted back and forth via I2C. This makes things **slower, less precise, and prone to errors**. To be safe from errors, you'll need to do some extra checking (see [Error handling](#Error handling)).
   - Naturally, you cannot declare your own stepping functions with the [constructor [2/2] variant](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#afa3061ce813303a8f2fa206ee8d012bd).

### Safety precautions

Steppers can exert damaging forces, even if they are moving slow. If in doubt, set up your system in a way that errors will not break things, particularly during testing:

* Place your end stops in a **non-blocking position** so that they are activated in a by-passing way but do not block the way themselves.
* To be really safe, put **emergency stops** which shut down the target in a final end position. Currently there is no dedicated pin mechanism for that, so just use the target's reset pin instead.
* Always keep a **manual emergency stop** at hand or make it easy to cut the power quickly.
* And again, do check for transmission errors in critical situations (see **[error handling](#error-handling)**).

## ServoI2C

Controls servo motors via I2C. Works just like the plain Arduino [`Servo`](https://www.arduino.cc/reference/en/libraries/servo) library. See [`Servo_Sweep.ino`](examples/Servo_Sweep/Servo_Sweep.ino) example.

## PinI2C

Read and control the digital, and analog, input/output pins of the target device via I2C. Can replace a dedicated digital or analog port expander like MCP23017, PCF8574, PCF8591, or ADS1115. Can be used like  the plain Arduino `digitalRead()` etc. commands. See [`Pin_control.ino`](examples/Pin_control/Pin_control.ino) example.

## ESP32sensorsI2C

Read an ESP32's touch sensors, hall sensor, and (if it works) temperature sensor via I2C. Can use the optional I2Cwrapper interrupt mechanism to inform the controller about a touch button press. See [`ESP32sensors.ino`](examples/ESP32sensors/ESP32sensors.ino) example.

# How to add new modules

You can use the templates provided in the [`templates` subfolder](templates) if you want to add your own modules and **implement your own I2C target device**.

- `template_I2C.h`  and `template_I2C.cpp` - controller library templates. Their main function is to define an interface for the target's functionality and the related command codes (see [limitations](#Limitations for module authors)). Each function is implemented so that the function's command code and parameters are transmitted to the target with the help of the I2Cwrapper library.
  - Often, the header file `template_I2C.h`  will very closely resemble the header file of the library that you are addressing on the target device's side.
  - The implementation  `template_I2C.cpp`, however, looks quite different: It will do nothing else but "wrap" each function's arguments into a command, transmit it to the target, and, optionally, receive the target device's reply.
- `template_I2C_firmware.h` - Target firmware templates. Here, the most important part is injecting code into the command interpreter that will "unwrap" the controller function's command codes, react adequately, and, optionally, prepare a reply.

Refer to the documentation within the templates' source code and to the [existing modules](src) for more details and illustration.

### A note on messages and "(sub)units"

All transmissions to the target device have a three byte header followed by an arbitrary number of zero or more parameter bytes.

- [0] **CRC8 checksum**
- [1] **command code**: Modules and the I2Cwrapper core use their own unique command code ranges, so that the command code will decide which module (if more than one is enabled) or if the core will interpret the command.
- [3] **unit addressed**: If a target module enables I2C access to more than one instance of some hardware, e.g. multiple stepper or servo motors, the unit can be used to differentiate them. It is up to each module to decide if and how the unit is interpreted. Modules which don't need them because there is only one instance of their respective hardware (like e.g. the `PinI2C` module), can just ignore the unit and will have to live with the one byte wasted bandwidth per transmission.

# Supported platforms

The following platforms will run the target firmware and have been (more or less) tested. Unfortunately, they all have their pros and cons:

* **Arduino AVRs (Uno, Nano etc.)**: Comes with I2C hardware support which should make communication most reliable and allows driving the I2C bus at higher frequencies. With only 16MHz CPU speed not recommended for high performance situations.
* **ESP8266**: Has no I2C  hardware. The software I2C will not work stable at the default 80MHz CPU speed, make sure to configure the **CPU clock speed to 160MHz**. Even then, it might be necessary to [decrease the bus speed](https://www.arduino.cc/en/Reference/WireSetClock) below 100kHz for stable bus performance, start as low as 10kHz if in doubt. Apart from that, expect a performance increase of ca. 10-15x vs. plain Arduinos due to higher CPU clock speed and better hardware support for math calculations.
* **ESP32**: Has no I2C  hardware. I2C is stable at the default 240MHz, but officially cannot run faster than 100kHz. Also, the target implementation is awkward. It might be more susceptible for I2C transmission errors, so [timing is critical](#Adjusting the I2C delay). Apart from that, expect a performance increase of ca. 15-20x vs. plain Arduinos due to higher CPU clock speed and better hardware support for math calculations.

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

I2Cwrapper wrapper(i2cAddress); // each target device is represented by a wrapper...
AccelStepperI2C stepper(&wrapper); // ...that the stepper needs to communicate with it

void setup()
{  
  Wire.begin();
  // Wire.setClock(10000); // uncomment for ESP8266 targets, to be on the safe side

  if (!wrapper.ping()) {
    Serial.println("Target not found! Check connections and restart.");
    while (true) {}
  }  
  wrapper.reset(); // reset the target device
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

[Find the I2Cwrapper library documentation here](https://ftjuh.github.io/I2Cwrapper/).

# Planned improvements

- Self-adjusting I2C-delay
- Determine I2C-address from hardware pins
- Move I2C-address options (fixed, EEPROM, hardware pins) to modules
- Attiny support (memory will be an issue, though)

# Author

Apart from its predecessor AccelStepperI2C, this is my first "serious" piece of software published on github. Although I've some background in programming, mostly in the Wirth-tradition languages, I'm far from being a competent or even avid c++ programmer. At the same time I have a tendency to over-engineer (not a good combination), so be warned and use this at your own risk. My current main interest is in 3D printing, you can find me on [prusaprinters](https://www.prusaprinters.org/social/202816-juh/about), [thingiverse](https://www.thingiverse.com/juh/designs), and [youmagine](https://www.youmagine.com/juh3d/designs). This library was developed as part of my [StepFish project](https://www.prusaprinters.org/prints/115049-stepfish-fischertechnik-i2c-stepper-motor-controll) ([also here](https://forum.ftcommunity.de/viewtopic.php?t=5341)).

Contact me at ftjuh@posteo.net.

Jan (juh)

# Copyright

This software is Copyright (C) 2022 juh (ftjuh@posteo.net)

# License

I2Cwrapper is distributed under the GNU GENERAL PUBLIC LICENSE Version 2.

# History

v0.3.0 Initial release, forked from AccelStepperI2C

Historical note: I2Cwrapper evolved from the [AccelStepperI2C project](https://github.com/ftjuh/AccelStepperI2C). The latter is still available in the Arduino library manager even if its use is discouraged. It is functionally fully equivalent to AccelSteperI2C if you simply select only this module for compilation and ignore the other modules.



