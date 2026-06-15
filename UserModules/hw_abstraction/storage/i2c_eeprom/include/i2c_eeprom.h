#ifndef USERMODULES_I2C_EEPROM_H
#define USERMODULES_I2C_EEPROM_H

#include <stdint.h>

#include "hw_time.h"
#include "i2c_transport.h"
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * block_address_bits maps high memory-address bits into the low bits of the
 * 7-bit I2C device address, as required by devices such as 24C04/08/16.
 */
typedef struct
{
    const char *name;
    I2cDevice device;
    HwTime time;
    uint64_t capacity_bytes;
    uint32_t page_size_bytes;
    uint32_t transfer_timeout_ms;
    uint32_t write_timeout_ms;
    uint8_t word_address_bytes;
    uint8_t block_address_bits;
} I2cEepromConfig;

typedef struct
{
    Storage base;
    I2cEepromConfig config;
} I2cEeprom;

StorageStatus i2c_eeprom_construct( I2cEeprom *eeprom,
                                    const I2cEepromConfig *config );
Storage *i2c_eeprom_as_storage( I2cEeprom *eeprom );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_I2C_EEPROM_H */
