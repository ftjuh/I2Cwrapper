#ifndef I2CwrapperVersion_h
#define I2CwrapperVersion_h

const uint8_t I2Cw_VersionMajor = 0;
const uint8_t I2Cw_VersionMinor = 1;
const uint8_t I2Cw_VersionPatch = 0;

const uint32_t I2Cw_Version = (uint32_t)I2Cw_VersionMajor << 16 | (uint32_t)I2Cw_VersionMinor << 8 | (uint32_t)I2Cw_VersionPatch;

#endif
