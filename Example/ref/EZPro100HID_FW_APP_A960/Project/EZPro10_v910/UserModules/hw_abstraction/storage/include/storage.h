#ifndef USERMODULES_STORAGE_H
#define USERMODULES_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "hw_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_CAP_READ           ( 1UL << 0 )
#define STORAGE_CAP_PROGRAM        ( 1UL << 1 )
#define STORAGE_CAP_ERASE          ( 1UL << 2 )
#define STORAGE_CAP_REQUIRES_ERASE ( 1UL << 3 )

typedef HwStatus StorageStatus;

#define STORAGE_OK                 HW_STATUS_OK
#define STORAGE_INVALID_ARGUMENT   HW_STATUS_INVALID_ARGUMENT
#define STORAGE_NOT_INITIALIZED    HW_STATUS_NOT_INITIALIZED
#define STORAGE_OUT_OF_RANGE       HW_STATUS_OUT_OF_RANGE
#define STORAGE_UNALIGNED          HW_STATUS_UNALIGNED
#define STORAGE_UNSUPPORTED        HW_STATUS_UNSUPPORTED
#define STORAGE_IO_ERROR           HW_STATUS_IO_ERROR
#define STORAGE_TIMEOUT            HW_STATUS_TIMEOUT
#define STORAGE_NOT_FOUND          HW_STATUS_NOT_FOUND
#define STORAGE_BUSY               HW_STATUS_BUSY
#define STORAGE_BAD_CONFIG         HW_STATUS_BAD_CONFIG

typedef struct Storage Storage;

typedef struct
{
    uint64_t capacity_bytes;
    uint32_t page_size_bytes;
    uint32_t erase_size_bytes;
    uint32_t capabilities;
    uint8_t erased_value;
} StorageInfo;

typedef struct
{
    StorageStatus ( *init )( Storage *storage );
    StorageStatus ( *read )( Storage *storage,
                             uint64_t address,
                             uint8_t *data,
                             size_t length );
    StorageStatus ( *program )( Storage *storage,
                                uint64_t address,
                                const uint8_t *data,
                                size_t length );
    StorageStatus ( *erase )( Storage *storage,
                              uint64_t address,
                              uint64_t length );
} StorageOps;

struct Storage
{
    const char *name;
    const StorageOps *ops;
    StorageInfo info;
    uint8_t initialized;
};

void storage_base_construct( Storage *storage,
                             const char *name,
                             const StorageOps *ops );
StorageStatus storage_init( Storage *storage );
StorageStatus storage_read( Storage *storage,
                            uint64_t address,
                            uint8_t *data,
                            size_t length );
StorageStatus storage_program( Storage *storage,
                               uint64_t address,
                               const uint8_t *data,
                               size_t length );
StorageStatus storage_erase( Storage *storage,
                             uint64_t address,
                             uint64_t length );
StorageStatus storage_get_info( const Storage *storage, StorageInfo *info );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_STORAGE_H */
