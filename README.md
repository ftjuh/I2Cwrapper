Warning: Pre-alpha! Not for production, only for feedback. Expect things to break, until stated otherwise.

## [AccelStepperI2C](https://github.com/ftjuh/AccelStepperI2C)

This is an I2C wrapper for Mike McCauley's [AccelStepper library](https://www.airspayce.com/mikem/arduino/AccelStepper/index.html) with additional support for two end stops per stepper. It consists of the AccelStepperI2C  Arduino-based firmware for one or more I2C-slaves, and a corresponding Arduino library for the I2C-master. Think of it as a more accessible and more flexible replacement for dedicated I2C stepper motor controller ICs like AMIS-30622, PCA9629 or TMC223.

### How does it work?

* One or more Arduino-likes, acting as **I2C-slaves**, run up to eight stepper motors each. The slaves run the AccelStepperI2C firmware (firmware.ino), which exposes the AccelStepper functionality via I2C. Steppers are driven by any method  supported by AccelStepper except AccelStepper::FUNCTION, e.g. AccelStepper::DRIVER with StepStick compatible  A4988, DRV8825 or TMC2100 stepper drivers. An obvious example is the Arduino UNO with a Protoneer CNC shield (recent [V3.51](https://www.elecrow.com/arduino-cnc-shield-v3-51-grbl-v0-9-compatible-uses-pololu-drivers.html) or [V3.00 clone](https://forum.protoneer.co.nz/viewforum.php?f=17)).
* Another device, acting as **I2C-master**, controls the slave(s) via I2C with the help of the AccelStepperI2C library. [Its interface](https://ftjuh.github.io/AccelStepperI2C/)  is largely identical to the original AccelStepper library.
* Refer to the AccelStepperI2C library's example folder to see how it works in detail.

### Differences from AccelStepper

   - With the exception of two blocking functions (see below), AccelStepper needs the client to constantly 'poll' each stepper by invoking one of the run() commands (run(), runSpeed(), or runSpeedToPosition()) at a frequency which mustn't be lower than the stepping frequency. Over I2C, this would clutter the bus, put limits on stepper speeds, and be unstable if there are other I2C devices on the bus, particularly with multiple steppers.
   - To solve this problem, the AccelStepperI2C firmware implements a __state machine__ for each connected stepper which makes the slave do the polling locally on its own.
   - All the master has to do is making the appropriate settings, e.g. setting a target with AccelStepperI2C::moveTo() or AccelStepperI2C::move(), or choosing a speed with AccelStepperI2C::setSpeed() and then start the slave's state machine with one of 
      + AccelStepperI2C::runState(): will poll run(), i.e. run to the target with acceleration, and stop 
        the state machine upon reaching it
      + AccelStepperI2C::runSpeedState(): will poll runSpeed(), i.e. run at constant speed until told otherwise (see AccelStepperI2C::stopState()), or
      + AccelStepperI2C::runSpeedToPositionState(): will poll runSpeedToPosition(), i.e. run at constant speed until target has been reached.
   - AccelStepperI2C::stopState() will stop any of the above states, i.e. stop polling. It does nothing else, so the master is solely in command of target, speed, and other settings.
   - Up to two **end stop switches** can be defined for each stepper. If enabled and the stepper runs into one of them, it will make the state machine stop just like AccelStepperI2C::stopState(). Of course, this is most useful in combination with ::runSpeedState() for homing tasks at startup.

### Restrictions

   - The original run(), runSpeed(), or runSpeedToPosition() are implemented, but I discourage using them directly. It's silly to trigger each stepper step over I2C. Use the state machine instead.
   - Each library function call, its parameters, and return value has to be transmitted back and forth via I2C. This makes things slower, less precise, and prone to errors.
   - The two blocking functions, runToPosition() and runToNewPositionCmd(), are not implemented at the moment. I never saw their purpose, anyway, as their behavior can easily implemented with the existing functionality. Also, naturally, you cannot declare your own stepping functions with the [constructor [2/2] variant](https://www.airspayce.com/mikem/arduino/AccelStepper/classAccelStepper.html#afa3061ce813303a8f2fa206ee8d012bd).
   - There is no interrupt mechanism at the moment which tells the master that a state machine job has finished or an endstop has been triggered. So the master still needs to poll the slave with one of AccelStepperI2C::distanceToGo(), AccelStepperI2C::currentPosition, AccelStepperI2C::isRunning(), or AccelStepperI2C::endstops() but can do so at a much more reasonable frequency.
   - No serialization protocol is used at the moment, so the implementation is machine dependent in regard to the endians and sizes of data types. Arduinos Uno/Nano and ESP8266 have been tested and work well together, as should any other Arduino supporting platform.
   - ~~Currently, there is no error checking of the master-slave communication, which uses a very basic (i.e. non existing) protocol. The master will have to deal with any transmission errors should they occur.~~

### Error handling

If I2C transmission problems occur, any call to the library could fail and, possibly worse, every return value could be corrupted. Uncontrolled or unexpected stepper movements could lead to serious problems. That's why each command and response is sent with a CRC8 checksum. To find out if a master's command or a slave's response was **transmitted correctly**, the master can check AccelStepperI2C::sentOK after each function call and AccelStepperI2C::resultOK after receiving data back from the slave device. 

- If AccelStepperI2C::sentOK is false, the function call was not properly transmitted. 
- If AccelStepperI2C::resultOK is false, the data returned from a function call is invalid.

### Usage

   1. Install the libraries AccelStepperI2C, SimpleBuffer, and of course the original AccelStepper to the Arduino environment.
   2. Upload firmware.ino to one Arduino (slave). Connect steppers and stepper drivers or use a dedicated hardware, e.g. Arduino UNO with CNC V3.00 shield.
   3. Upload example sketch or your own sketch which uses the AccelStepperI2C library to another Arduino-like (master).
   4. Connect the I2C bus of both devices (usually A4<->A4, A5<->A5, GND<->GND). Don't fortget two I2C pullups and, if needed, level-shifters. Also connect +5V<->+5V to power one board from the other, if needed.
   7. Now (not earlier) provide external power to the steppers and power to the Arduinos.

Have a look at the examples for usage of the library. A couple of important things: 

* The AccelStepperI2C constructor needs the Wire library to be already initialized. So don't invoke the constructor when declaring an object, use **new** during setup().
* 

### Documentation

[Find the AccelStepperI2C library documentation here](https://ftjuh.github.io/AccelStepperI2C/). It only deals with differences from running the AccelStepper library locally. If not stated otherwise, expect [functions and parameters](@ref AccelStepperI2C) to work as in the original, keeping the above differences and restrictions in mind.

### Author

This is my first "serious" piece of software published on github. Although I've some background in programming, mostly in the Wirth-tradition languages, I'm far from being a competent or even avid c++ programmer. At the same time I have a tendency to over-engineer (not a good combination), so be warned and use this at your own risk. My current main interest is in 3D printing, you can find me on [prusaprinters](https://www.prusaprinters.org/social/202816-juh/about), [thingiverse](https://www.thingiverse.com/juh/designs), and [youmagine](https://www.youmagine.com/juh3d/designs). This library was developed as part of my [StepFish project](https://www.prusaprinters.org/prints/115049-stepfish-fischertechnik-i2c-stepper-motor-controll) ([also here](https://forum.ftcommunity.de/viewtopic.php?t=5341)).

Contact me at juh@posteo.org.

Jan (juh)

### Copyright

This software is Copyright (C) 2022 juh (juh@posteo.org)

### License

AccelStepperI2C is distributed under the GNU GENERAL PUBLIC LICENSE Version 2.

### History

v0.1 Initial release

