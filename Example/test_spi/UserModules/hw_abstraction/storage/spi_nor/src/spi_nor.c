#include "spi_nor.h"

#include <string.h>

#include "container_of.h"

#define SPI_NOR_JEDEC_ID_OPCODE    0x9FU
#define SPI_NOR_POLL_DELAY_MS      1U

#define SPI_NOR_ERASE_4K \
    { 4UL * 1024UL, 400U, 0x20U }
#define SPI_NOR_ERASE_32K \
    { 32UL * 1024UL, 1600U, 0x52U }
#define SPI_NOR_ERASE_64K \
    { 64UL * 1024UL, 2000U, 0xD8U }

#define W25Q_PROFILE(model_name, capacity_code, capacity, address_count, enter_opcode) \
    { \
        ( model_name ), \
        { 0xEFU, 0x40U, ( capacity_code ) }, \
        ( capacity ), \
        256U, \
        20U, \
        ( address_count ), \
        0x03U, \
        0x02U, \
        0x06U, \
        0x05U, \
        0x01U, \
        ( enter_opcode ), \
        { SPI_NOR_ERASE_4K, SPI_NOR_ERASE_32K, SPI_NOR_ERASE_64K }, \
        3U \
    }

static const SpiNorProfile default_profiles[] =
{
    W25Q_PROFILE( "W25Q16", 0x15U, 2ULL * 1024ULL * 1024ULL, 3U, 0U ),
    W25Q_PROFILE( "W25Q32", 0x16U, 4ULL * 1024ULL * 1024ULL, 3U, 0U ),
    W25Q_PROFILE( "W25Q64", 0x17U, 8ULL * 1024ULL * 1024ULL, 3U, 0U ),
    W25Q_PROFILE( "W25Q128", 0x18U, 16ULL * 1024ULL * 1024ULL, 3U, 0U ),
    W25Q_PROFILE( "W25Q256", 0x19U, 32ULL * 1024ULL * 1024ULL, 4U, 0xB7U ),
    W25Q_PROFILE( "W25Q512", 0x20U, 64ULL * 1024ULL * 1024ULL, 4U, 0xB7U )
};

static StorageStatus nor_init( Storage *storage );
static StorageStatus nor_read( Storage *storage,
                               uint64_t address,
                               uint8_t *data,
                               size_t length );
static StorageStatus nor_program( Storage *storage,
                                  uint64_t address,
                                  const uint8_t *data,
                                  size_t length );
static StorageStatus nor_erase( Storage *storage,
                                uint64_t address,
                                uint64_t length );

static const StorageOps nor_ops =
{
    nor_init,
    nor_read,
    nor_program,
    nor_erase
};

static void encode_address( uint8_t *output,
                            uint64_t address,
                            uint8_t address_bytes )
{
    uint8_t index;

    for( index = 0U; index < address_bytes; index++ )
    {
        uint8_t shift = ( uint8_t ) ( ( address_bytes - index - 1U ) * 8U );
        output[ index ] = ( uint8_t ) ( address >> shift );
    }
}

static StorageStatus send_command( SpiNor *nor, uint8_t opcode )
{
    SpiSegment segment;

    segment.tx_data = &opcode;
    segment.rx_data = 0;
    segment.length = 1U;
    segment.fill_byte = 0xFFU;

    return spi_transfer( &nor->config.device,
                         &segment,
                         1U,
                         nor->config.transfer_timeout_ms );
}

static StorageStatus read_status( SpiNor *nor, uint8_t *status_value )
{
    uint8_t opcode = nor->profile->read_status_opcode;
    SpiSegment segments[ 2 ];

    segments[ 0 ].tx_data = &opcode;
    segments[ 0 ].rx_data = 0;
    segments[ 0 ].length = 1U;
    segments[ 0 ].fill_byte = 0xFFU;
    segments[ 1 ].tx_data = 0;
    segments[ 1 ].rx_data = status_value;
    segments[ 1 ].length = 1U;
    segments[ 1 ].fill_byte = 0xFFU;

    return spi_transfer( &nor->config.device,
                         segments,
                         2U,
                         nor->config.transfer_timeout_ms );
}

static StorageStatus wait_ready( SpiNor *nor, uint32_t timeout_ms )
{
    uint32_t start_ms = nor->config.time.get_time_ms( nor->config.time.ctx );

    for( ;; )
    {
        uint8_t status_value = 0U;
        StorageStatus status = read_status( nor, &status_value );

        if( status != STORAGE_OK )
        {
            return status;
        }

        if( ( status_value & nor->profile->busy_mask ) == 0U )
        {
            return STORAGE_OK;
        }

        if( hw_time_elapsed_ms(
                start_ms,
                nor->config.time.get_time_ms( nor->config.time.ctx ) ) >=
            timeout_ms )
        {
            return STORAGE_TIMEOUT;
        }

        nor->config.time.delay_ms( nor->config.time.ctx,
                                   SPI_NOR_POLL_DELAY_MS );
    }
}

static const SpiNorProfile *find_profile( const SpiNor *nor,
                                         const uint8_t jedec_id[ 3 ] )
{
    size_t index;
    const SpiNorProfile *profiles = nor->config.profiles;
    size_t profile_count = nor->config.profile_count;

    if( profiles == 0 )
    {
        profiles = spi_nor_default_profiles( &profile_count );
    }

    for( index = 0U; index < profile_count; index++ )
    {
        if( memcmp( profiles[ index ].jedec_id, jedec_id, 3U ) == 0 )
        {
            return &profiles[ index ];
        }
    }

    return 0;
}

static int profile_is_valid( const SpiNorProfile *profile )
{
    size_t index;
    uint32_t minimum_erase_size;
    uint64_t addressable_capacity;

    if( ( profile == 0 ) ||
        ( profile->capacity_bytes == 0U ) ||
        ( profile->page_size_bytes == 0U ) ||
        ( profile->program_timeout_ms == 0U ) ||
        ( profile->address_bytes < 1U ) ||
        ( profile->address_bytes > 4U ) ||
        ( profile->read_opcode == 0U ) ||
        ( profile->page_program_opcode == 0U ) ||
        ( profile->write_enable_opcode == 0U ) ||
        ( profile->read_status_opcode == 0U ) ||
        ( profile->busy_mask == 0U ) ||
        ( profile->erase_type_count == 0U ) ||
        ( profile->erase_type_count > SPI_NOR_MAX_ERASE_TYPES ) )
    {
        return 0;
    }

    addressable_capacity = 1ULL << ( profile->address_bytes * 8U );
    if( ( profile->capacity_bytes > addressable_capacity ) ||
        ( profile->page_size_bytes > profile->capacity_bytes ) )
    {
        return 0;
    }

    minimum_erase_size = profile->erase_types[ 0 ].size_bytes;
    for( index = 0U; index < profile->erase_type_count; index++ )
    {
        if( ( profile->erase_types[ index ].size_bytes == 0U ) ||
            ( profile->erase_types[ index ].size_bytes >
              profile->capacity_bytes ) ||
            ( profile->erase_types[ index ].timeout_ms == 0U ) ||
            ( profile->erase_types[ index ].opcode == 0U ) )
        {
            return 0;
        }

        if( profile->erase_types[ index ].size_bytes < minimum_erase_size )
        {
            minimum_erase_size = profile->erase_types[ index ].size_bytes;
        }
    }

    if( ( profile->capacity_bytes % minimum_erase_size ) != 0U )
    {
        return 0;
    }

    for( index = 0U; index < profile->erase_type_count; index++ )
    {
        if( ( profile->erase_types[ index ].size_bytes %
              minimum_erase_size ) != 0U )
        {
            return 0;
        }
    }

    return 1;
}

static StorageStatus nor_init( Storage *storage )
{
    SpiNor *nor = container_of( storage, SpiNor, base );
    uint8_t opcode = SPI_NOR_JEDEC_ID_OPCODE;
    uint8_t jedec_id[ 3 ] = { 0U, 0U, 0U };
    SpiSegment segments[ 2 ];
    StorageStatus status;
    uint32_t minimum_erase_size;
    size_t index;

    segments[ 0 ].tx_data = &opcode;
    segments[ 0 ].rx_data = 0;
    segments[ 0 ].length = 1U;
    segments[ 0 ].fill_byte = 0xFFU;
    segments[ 1 ].tx_data = 0;
    segments[ 1 ].rx_data = jedec_id;
    segments[ 1 ].length = sizeof( jedec_id );
    segments[ 1 ].fill_byte = 0xFFU;

    status = spi_transfer( &nor->config.device,
                           segments,
                           2U,
                           nor->config.transfer_timeout_ms );
    if( status != STORAGE_OK )
    {
        return status;
    }

    nor->profile = find_profile( nor, jedec_id );
    if( nor->profile == 0 )
    {
        return STORAGE_NOT_FOUND;
    }

    if( !profile_is_valid( nor->profile ) )
    {
        nor->profile = 0;
        return STORAGE_BAD_CONFIG;
    }

    if( nor->profile->enter_4byte_opcode != 0U )
    {
        status = send_command( nor, nor->profile->enter_4byte_opcode );
        if( status != STORAGE_OK )
        {
            nor->profile = 0;
            return status;
        }
    }

    minimum_erase_size = nor->profile->erase_types[ 0 ].size_bytes;
    for( index = 1U; index < nor->profile->erase_type_count; index++ )
    {
        if( nor->profile->erase_types[ index ].size_bytes <
            minimum_erase_size )
        {
            minimum_erase_size =
                nor->profile->erase_types[ index ].size_bytes;
        }
    }

    storage->info.capacity_bytes = nor->profile->capacity_bytes;
    storage->info.page_size_bytes = nor->profile->page_size_bytes;
    storage->info.erase_size_bytes = minimum_erase_size;
    storage->info.erased_value = 0xFFU;
    storage->info.capabilities = STORAGE_CAP_READ |
                                 STORAGE_CAP_PROGRAM |
                                 STORAGE_CAP_ERASE |
                                 STORAGE_CAP_REQUIRES_ERASE;
    return STORAGE_OK;
}

static StorageStatus nor_read( Storage *storage,
                               uint64_t address,
                               uint8_t *data,
                               size_t length )
{
    SpiNor *nor = container_of( storage, SpiNor, base );
    uint8_t command[ 5 ];
    SpiSegment segments[ 2 ];

    if( length == 0U )
    {
        return STORAGE_OK;
    }

    command[ 0 ] = nor->profile->read_opcode;
    encode_address( &command[ 1 ], address, nor->profile->address_bytes );

    segments[ 0 ].tx_data = command;
    segments[ 0 ].rx_data = 0;
    segments[ 0 ].length = ( size_t ) nor->profile->address_bytes + 1U;
    segments[ 0 ].fill_byte = 0xFFU;
    segments[ 1 ].tx_data = 0;
    segments[ 1 ].rx_data = data;
    segments[ 1 ].length = length;
    segments[ 1 ].fill_byte = 0xFFU;

    return spi_transfer( &nor->config.device,
                         segments,
                         2U,
                         nor->config.transfer_timeout_ms );
}

static StorageStatus program_page( SpiNor *nor,
                                   uint64_t address,
                                   const uint8_t *data,
                                   size_t length )
{
    uint8_t command[ 5 ];
    SpiSegment segments[ 2 ];
    StorageStatus status;

    status = send_command( nor, nor->profile->write_enable_opcode );
    if( status != STORAGE_OK )
    {
        return status;
    }

    command[ 0 ] = nor->profile->page_program_opcode;
    encode_address( &command[ 1 ], address, nor->profile->address_bytes );

    segments[ 0 ].tx_data = command;
    segments[ 0 ].rx_data = 0;
    segments[ 0 ].length = ( size_t ) nor->profile->address_bytes + 1U;
    segments[ 0 ].fill_byte = 0xFFU;
    segments[ 1 ].tx_data = data;
    segments[ 1 ].rx_data = 0;
    segments[ 1 ].length = length;
    segments[ 1 ].fill_byte = 0xFFU;

    status = spi_transfer( &nor->config.device,
                           segments,
                           2U,
                           nor->config.transfer_timeout_ms );
    return ( status == STORAGE_OK ) ?
           wait_ready( nor, nor->profile->program_timeout_ms ) :
           status;
}

static StorageStatus nor_program( Storage *storage,
                                  uint64_t address,
                                  const uint8_t *data,
                                  size_t length )
{
    SpiNor *nor = container_of( storage, SpiNor, base );

    while( length != 0U )
    {
        uint32_t page_offset =
            ( uint32_t ) ( address % nor->profile->page_size_bytes );
        size_t chunk =
            ( size_t ) ( nor->profile->page_size_bytes - page_offset );
        StorageStatus status;

        if( chunk > length )
        {
            chunk = length;
        }

        status = program_page( nor, address, data, chunk );
        if( status != STORAGE_OK )
        {
            return status;
        }

        address += chunk;
        data += chunk;
        length -= chunk;
    }

    return STORAGE_OK;
}

static const SpiNorEraseType *select_erase_type( const SpiNorProfile *profile,
                                                 uint64_t address,
                                                 uint64_t remaining )
{
    const SpiNorEraseType *selected = 0;
    size_t index;

    for( index = 0U; index < profile->erase_type_count; index++ )
    {
        const SpiNorEraseType *candidate = &profile->erase_types[ index ];

        if( ( remaining >= candidate->size_bytes ) &&
            ( ( address % candidate->size_bytes ) == 0U ) &&
            ( ( selected == 0 ) ||
              ( candidate->size_bytes > selected->size_bytes ) ) )
        {
            selected = candidate;
        }
    }

    return selected;
}

static StorageStatus erase_block( SpiNor *nor,
                                  uint64_t address,
                                  const SpiNorEraseType *erase_type )
{
    uint8_t command[ 5 ];
    SpiSegment segment;
    StorageStatus status;

    status = send_command( nor, nor->profile->write_enable_opcode );
    if( status != STORAGE_OK )
    {
        return status;
    }

    command[ 0 ] = erase_type->opcode;
    encode_address( &command[ 1 ], address, nor->profile->address_bytes );
    segment.tx_data = command;
    segment.rx_data = 0;
    segment.length = ( size_t ) nor->profile->address_bytes + 1U;
    segment.fill_byte = 0xFFU;

    status = spi_transfer( &nor->config.device,
                           &segment,
                           1U,
                           nor->config.transfer_timeout_ms );
    return ( status == STORAGE_OK ) ?
           wait_ready( nor, erase_type->timeout_ms ) :
           status;
}

static StorageStatus nor_erase( Storage *storage,
                                uint64_t address,
                                uint64_t length )
{
    SpiNor *nor = container_of( storage, SpiNor, base );
    uint32_t minimum_size = storage->info.erase_size_bytes;

    if( length == 0U )
    {
        return STORAGE_OK;
    }

    if( ( ( address % minimum_size ) != 0U ) ||
        ( ( length % minimum_size ) != 0U ) )
    {
        return STORAGE_UNALIGNED;
    }

    while( length != 0U )
    {
        const SpiNorEraseType *erase_type =
            select_erase_type( nor->profile, address, length );
        StorageStatus status;

        if( erase_type == 0 )
        {
            return STORAGE_BAD_CONFIG;
        }

        status = erase_block( nor, address, erase_type );
        if( status != STORAGE_OK )
        {
            return status;
        }

        address += erase_type->size_bytes;
        length -= erase_type->size_bytes;
    }

    return STORAGE_OK;
}

StorageStatus spi_nor_construct( SpiNor *nor, const SpiNorConfig *config )
{
    if( ( nor == 0 ) ||
        ( config == 0 ) ||
        ( config->device.bus == 0 ) ||
        !hw_time_is_valid( &config->time ) ||
        ( config->transfer_timeout_ms == 0U ) ||
        ( ( config->profiles == 0 ) && ( config->profile_count != 0U ) ) ||
        ( ( config->profiles != 0 ) && ( config->profile_count == 0U ) ) )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    memset( nor, 0, sizeof( *nor ) );
    nor->config = *config;
    storage_base_construct( &nor->base, config->name, &nor_ops );
    return STORAGE_OK;
}

Storage *spi_nor_as_storage( SpiNor *nor )
{
    return ( nor != 0 ) ? &nor->base : 0;
}

const SpiNorProfile *spi_nor_default_profiles( size_t *profile_count )
{
    if( profile_count != 0 )
    {
        *profile_count = sizeof( default_profiles ) /
                         sizeof( default_profiles[ 0 ] );
    }

    return default_profiles;
}
