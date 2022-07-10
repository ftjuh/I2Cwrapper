/*!
   @file _addressFixed_firmware.h

   @brief Feature module.
   Defines a fixed address, other than the default 0x08, for the target.

   ## Author
   Copyright (c) 2022 juh
   ## License
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, version 2.
*/
/// @cond

/*
   (2) declarations
*/

#if MF_STAGE == MF_STAGE_declarations

// claim and define address retrieval for this module
#ifndef I2C_ADDRESS_DEFINED_BY_MODULE
#define I2C_ADDRESS_DEFINED_BY_MODULE 11 // any valid address >= 0x8 and <= 0x77
#else
#error 'More than one address defining feature module enabled in firmware_modules.h. Plaese deactivate all but one.'
#endif


#endif // MF_STAGE_declarations




/// @endcond
