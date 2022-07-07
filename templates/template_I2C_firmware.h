/*!
 * @file template_I2C_firmware.h
 * @brief Template for a user firmware module for the I2Cwrapper @ref
 * firmware.ino. "xxx" represents the name of your module, e.g. "PinI2C"
 *
 * Use this file as a template for your own optional extension module of 
 * the I2Cwrapper firmware. You will usually want to bundle it with a matching 
 * controller library (see template_I2C.h|cpp). Don't forget to copy it (I use
 * symlinks) into the firmware folder and activate it in the firmware_modules.h 
 * configuration file before compiling the firmware.
 * 
 * Currently, your code can be injected between the respective 
 * "#if MF_STAGE == ...." compiler directives (leave those untouched) at the 
 * following stages into the firmware framework:
 * 
 * (1) includes
 * (2) declarations
 * (3) setup() function
 * (4) loop() function
 * (5) processMessage() function ("command interpreter")
 * (6) reset event
 * (7) (end of) receiveEvent()
 * (8) (end of) requestEvent()
 * (9) Change of I2C state machine's state
 * 
 * Many modules use only a small subset of these stages. (1), (2), (5) are
 * probably always necessary for normal (non feature) modules.
 * 
 * See below for instructions on what code to place where. See existing modules
 * for illustration. PinI2C and ServoI2C are good and simple starting points, 
 * ESP32sensorsI2C for using interrupts, AccelStepperI2C for code injection 
 * into the main loop().
 * 
 * You can use the log() macro for debug output. It will produce no code with
 * DEBUG unset, and be extended into "Serial.print()" with DEBUG set.
 * 
 * ## Author
 * Copyright (c) 2022 juh
 * ## License
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2.
 */

/*
 * Note: Code injection will confuse doxygen, as withouth the including 
 * firmware.ino it looks like incomplete code. So, to prevent it from generating 
 * all kinds of gibberish documentation, the rest of this file is enclosed in 
 * cond/endcond tags to make doxygen ignore it. Usually, there's nothing of
 * interest here to document for end users anyway. If you feel otherwise, just 
 * enable doxygen for the relevant parts by enclosing them in endcond/cond 
 * tags.
 */

/// @cond

/*
 * (1) includes
 * 
 * Place include calls needed by your module here. Usually this will be at least
 * the definition file of your controller's library which defines the command codes 
 * used below and other relevant stuff.
 * 
 */
#if MF_STAGE == MF_STAGE_includes
#include <xxxI2C.h> // controller's library header file, needed here for the command codes 'xxxCmd' etc.
#endif // MF_STAGE_includes


/*
 * (2) declarations
 * 
 * This code will be injected after the include section and before the setup 
 * function. Use it to declare variables, constants and supporting functions 
 * needed by your module
 * 
 */
#if MF_STAGE == MF_STAGE_declarations

#endif // MF_STAGE_declarations


/*
 * (3) setup() function
 * 
 * This code will be injected into the target's setup() function.
 * 
 */
#if MF_STAGE == MF_STAGE_setup
log("###template### module enabled.\n");
#endif // MF_STAGE_setup


/*
 * (4) main loop() function
 * 
 * This code will be injected into the target's loop() function.
 * You may use triggerInterrupt() here to inform the controller about some 
 * event, if your module supports the interrupt mechanism.
 * 
 */
#if MF_STAGE == MF_STAGE_loop
#endif // MF_STAGE_loop


/*
 * (5) processMessage() function
 * 
 * This code will be injected into the processMessage() function's main switch{}
 * statement. It is responsible for reading the controller's commmands, processing 
 * them and, optionally for non void function calls from the controller, prepare 
 * some data to be sent back to the controller.
 * 
 * The code consists of 0 to n instances of "case xxxCmd: {}" clauses which can 
 * use the following constants and variables:
 * 
 * const uint8_t xxxCmd -> Command ID, usually defined by you as constant in 
 * the controller library's header file which has been included above in the 
 * MF_STAGE_includes stage. 
 * 
 * uint8_t unit -> The "unit" this command is adressed to (optional), e.g. which
 * of several stepper motors attached to the target. It's up to you how to
 * interpret this (see README.md). Will only be needed if your target device 
 * administers access to multiple instances of some hardware (e.g. steppers, 
 * servos; see PinsI2C and ESP32sensors for examples which don't use units).
 * 
 * uint8_t i -> The number of paramter bytes. Check this against the expected 
 * number of parameter bytes for each command to avoid interpreting garbage.
 * 
 * SimpleBuffer bufferIn() -> Read the command's parameters from here, see below
 * for examples.
 * 
 * SimpleBuffer bufferOut() -> If the command is non void, write its reply to
 * this buffer, see below for examples.
 * 
 */
#if MF_STAGE == MF_STAGE_processMessage

  case xxxCmd /* as defined in your controller's library header file */: {
    if (i == /* parameter bytes this commands needs */) { 
      
      /* use this to read the defined parameters for this command from 
       * the input buffer */
      uint8_t testIn /* can be any data type */; 
      bufferIn->read(testIn); /* will read sizeof(testIn) bytes from the buffer*/
      
      /* now the target should probably do sth meaningful with "testIn" */
      uint8_t testOut = testIn++;
      
      /* (optional) Use this to write a command's "reply" to the output 
       * buffer which will be sent upon the controller's next request event. 
       * Accepts any data type. */
      bufferOut->write(testOut);
      
    } 
  }
  break;

  // more "case xxxCmd: {}" clauses go here
        
#endif // MF_STAGE_processMessage
  
        
/*
 * (6) reset event
 * 
 * This code will be called if the target device is to perform a (soft) reset 
 * via I2C command. Here, the module needs to free any ressources it has allocated 
 * and put any hardware under its rule (and only that!) in its initial state. 
 * 
 * Note: From v0.3.0  on, no hardware reset is performed any more. So it is 
 * essential that after calling this code, the target device presents itself as 
 * a clean slate to the controller, just like after power up.
 */
#if MF_STAGE == MF_STAGE_reset
#endif // MF_STAGE_reset


/*
 * (7) receiveEvent()
 * 
 * Normal modules usually should not mess around here.
 * 
 */
#if MF_STAGE == MF_STAGE_receiveEvent
#endif // MF_STAGE_receiveEvent


/*
 * (8) requestEvent()
 * 
 * Normal modules usually should not mess around here.
 * 
 */
#if MF_STAGE == MF_STAGE_requestEvent
#endif // MF_STAGE_requestEvent


/*
 * (9) Change of I2C state machine's state
 * 
 * Normal modules usually should not mess around here.
 * 
 */

#if MF_STAGE == MF_STAGE_I2CstateChange
#endif // MF_STAGE_I2CstateChange




/// @endcond

