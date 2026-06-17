#ifndef USERMODULES_SPI_NOR_H
#define USERMODULES_SPI_NOR_H

#include <stddef.h>
#include <stdint.h>

#include "hw_time.h"
#include "spi_transport.h"
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_NOR_MAX_ERASE_TYPES    3U

typedef struct
{
    uint32_t size_bytes;
    uint32_t timeout_ms;
    uint8_t opcode;
} SpiNorEraseType;

typedef struct
{
    uint8_t opcode;
    uint8_t address_bytes;
    uint32_t address;
    uint8_t length;
} SpiNorIdProbe;

typedef struct
{
    uint8_t write_enable_opcode;
    uint8_t write_opcode;
    uint8_t value;
    uint32_t timeout_ms;
} SpiNorStatusInit;

/*
 * JEDEC-ID-selected command and geometry profile.
 * erase_types may be in any order; the driver chooses the largest valid type.
 */
typedef struct
{
    const char *model;
    uint8_t jedec_id[ 3 ];
    uint64_t capacity_bytes;
    uint32_t page_size_bytes;
    uint32_t program_timeout_ms;
    uint8_t address_bytes;
    uint8_t read_opcode;
    uint8_t page_program_opcode;
    uint8_t write_enable_opcode;
    uint8_t read_status_opcode;
    uint8_t busy_mask;
    uint8_t enter_4byte_opcode;
    SpiNorEraseType erase_types[ SPI_NOR_MAX_ERASE_TYPES ];
    size_t erase_type_count;
    SpiNorIdProbe id_probe;
    SpiNorStatusInit status_init;
} SpiNorProfile;

typedef struct
{
    const char *name;
    SpiDevice device;
    HwTime time;
    const SpiNorProfile *profiles;
    size_t profile_count;
    uint32_t transfer_timeout_ms;
} SpiNorConfig;

typedef struct
{
    Storage base;
    SpiNorConfig config;
    const SpiNorProfile *profile;
} SpiNor;

StorageStatus spi_nor_construct( SpiNor *nor, const SpiNorConfig *config );
Storage *spi_nor_as_storage( SpiNor *nor );
const SpiNorProfile *spi_nor_default_profiles( size_t *profile_count );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_SPI_NOR_H */
