#include "storage.h"

#include <string.h>

static StorageStatus storage_validate_range( const Storage *storage,
                                             uint64_t address,
                                             uint64_t length )
{
    if( length == 0U )
    {
        return STORAGE_OK;
    }

    if( ( address >= storage->info.capacity_bytes ) ||
        ( length > ( storage->info.capacity_bytes - address ) ) )
    {
        return STORAGE_OUT_OF_RANGE;
    }

    return STORAGE_OK;
}

void storage_base_construct( Storage *storage,
                             const char *name,
                             const StorageOps *ops )
{
    if( storage != 0 )
    {
        storage->name = name;
        storage->ops = ops;
        memset( &storage->info, 0, sizeof( storage->info ) );
        storage->initialized = 0U;
    }
}

StorageStatus storage_init( Storage *storage )
{
    StorageStatus status;

    if( ( storage == 0 ) ||
        ( storage->ops == 0 ) ||
        ( storage->ops->init == 0 ) )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    storage->initialized = 0U;
    status = storage->ops->init( storage );

    if( status == STORAGE_OK )
    {
        storage->initialized = 1U;
    }

    return status;
}

StorageStatus storage_read( Storage *storage,
                            uint64_t address,
                            uint8_t *data,
                            size_t length )
{
    StorageStatus status;

    if( ( storage == 0 ) || ( ( data == 0 ) && ( length != 0U ) ) )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    if( storage->initialized == 0U )
    {
        return STORAGE_NOT_INITIALIZED;
    }

    if( ( storage->ops == 0 ) || ( storage->ops->read == 0 ) )
    {
        return STORAGE_UNSUPPORTED;
    }

    status = storage_validate_range( storage, address, ( uint64_t ) length );
    return ( status == STORAGE_OK ) ?
           storage->ops->read( storage, address, data, length ) :
           status;
}

StorageStatus storage_program( Storage *storage,
                               uint64_t address,
                               const uint8_t *data,
                               size_t length )
{
    StorageStatus status;

    if( ( storage == 0 ) || ( ( data == 0 ) && ( length != 0U ) ) )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    if( storage->initialized == 0U )
    {
        return STORAGE_NOT_INITIALIZED;
    }

    if( ( storage->ops == 0 ) || ( storage->ops->program == 0 ) )
    {
        return STORAGE_UNSUPPORTED;
    }

    status = storage_validate_range( storage, address, ( uint64_t ) length );
    return ( status == STORAGE_OK ) ?
           storage->ops->program( storage, address, data, length ) :
           status;
}

StorageStatus storage_erase( Storage *storage,
                             uint64_t address,
                             uint64_t length )
{
    StorageStatus status;

    if( storage == 0 )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    if( storage->initialized == 0U )
    {
        return STORAGE_NOT_INITIALIZED;
    }

    if( ( storage->ops == 0 ) || ( storage->ops->erase == 0 ) )
    {
        return STORAGE_UNSUPPORTED;
    }

    status = storage_validate_range( storage, address, length );
    return ( status == STORAGE_OK ) ?
           storage->ops->erase( storage, address, length ) :
           status;
}

StorageStatus storage_get_info( const Storage *storage, StorageInfo *info )
{
    if( ( storage == 0 ) || ( info == 0 ) )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    if( storage->initialized == 0U )
    {
        return STORAGE_NOT_INITIALIZED;
    }

    *info = storage->info;
    return STORAGE_OK;
}
