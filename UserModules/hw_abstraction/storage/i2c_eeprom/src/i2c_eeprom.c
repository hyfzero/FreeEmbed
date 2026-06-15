#include "i2c_eeprom.h"

#include <string.h>

#include "container_of.h"

#define I2C_EEPROM_POLL_DELAY_MS    1U

static StorageStatus eeprom_init( Storage *storage );
static StorageStatus eeprom_read( Storage *storage,
                                  uint64_t address,
                                  uint8_t *data,
                                  size_t length );
static StorageStatus eeprom_program( Storage *storage,
                                     uint64_t address,
                                     const uint8_t *data,
                                     size_t length );

static const StorageOps eeprom_ops =
{
    eeprom_init,
    eeprom_read,
    eeprom_program,
    0
};

static uint64_t block_size( const I2cEeprom *eeprom )
{
    return 1ULL << ( eeprom->config.word_address_bytes * 8U );
}

static I2cDevice addressed_device( const I2cEeprom *eeprom,
                                   uint64_t address )
{
    I2cDevice device = eeprom->config.device;

    if( eeprom->config.block_address_bits != 0U )
    {
        uint16_t mask =
            ( uint16_t ) ( ( 1U << eeprom->config.block_address_bits ) - 1U );
        uint16_t block =
            ( uint16_t ) ( ( address / block_size( eeprom ) ) & mask );

        device.address =
            ( uint16_t ) ( ( device.address & ( uint16_t ) ~mask ) | block );
    }

    return device;
}

static void encode_word_address( const I2cEeprom *eeprom,
                                 uint64_t address,
                                 uint8_t output[ 2 ] )
{
    uint16_t word_address =
        ( uint16_t ) ( address % block_size( eeprom ) );

    if( eeprom->config.word_address_bytes == 2U )
    {
        output[ 0 ] = ( uint8_t ) ( word_address >> 8 );
        output[ 1 ] = ( uint8_t ) word_address;
    }
    else
    {
        output[ 0 ] = ( uint8_t ) word_address;
    }
}

static size_t limit_to_block( const I2cEeprom *eeprom,
                              uint64_t address,
                              size_t requested )
{
    uint64_t size = block_size( eeprom );
    uint64_t remaining = size - ( address % size );

    return ( remaining < requested ) ? ( size_t ) remaining : requested;
}

static StorageStatus wait_write_complete( I2cEeprom *eeprom,
                                          const I2cDevice *device )
{
    uint32_t start_ms =
        eeprom->config.time.get_time_ms( eeprom->config.time.ctx );

    for( ;; )
    {
        StorageStatus status =
            i2c_probe( device, eeprom->config.transfer_timeout_ms );

        if( status == STORAGE_OK )
        {
            return STORAGE_OK;
        }

        if( ( status != STORAGE_NOT_FOUND ) && ( status != STORAGE_BUSY ) )
        {
            return status;
        }

        if( hw_time_elapsed_ms(
                start_ms,
                eeprom->config.time.get_time_ms( eeprom->config.time.ctx ) ) >=
            eeprom->config.write_timeout_ms )
        {
            return STORAGE_TIMEOUT;
        }

        eeprom->config.time.delay_ms( eeprom->config.time.ctx,
                                      I2C_EEPROM_POLL_DELAY_MS );
    }
}

static StorageStatus eeprom_init( Storage *storage )
{
    I2cEeprom *eeprom = container_of( storage, I2cEeprom, base );
    StorageStatus status =
        i2c_probe( &eeprom->config.device,
                   eeprom->config.transfer_timeout_ms );

    if( status != STORAGE_OK )
    {
        return status;
    }

    storage->info.capacity_bytes = eeprom->config.capacity_bytes;
    storage->info.page_size_bytes = eeprom->config.page_size_bytes;
    storage->info.erase_size_bytes = 0U;
    storage->info.erased_value = 0xFFU;
    storage->info.capabilities = STORAGE_CAP_READ | STORAGE_CAP_PROGRAM;
    return STORAGE_OK;
}

static StorageStatus eeprom_read_chunk( I2cEeprom *eeprom,
                                        uint64_t address,
                                        uint8_t *data,
                                        size_t length )
{
    uint8_t word_address[ 2 ];
    I2cSegment address_segment;
    I2cSegment data_segment;
    I2cMessage messages[ 2 ];
    I2cDevice device = addressed_device( eeprom, address );

    encode_word_address( eeprom, address, word_address );
    address_segment.tx_data = word_address;
    address_segment.rx_data = 0;
    address_segment.length = eeprom->config.word_address_bytes;
    data_segment.tx_data = 0;
    data_segment.rx_data = data;
    data_segment.length = length;

    messages[ 0 ].segments = &address_segment;
    messages[ 0 ].segment_count = 1U;
    messages[ 0 ].flags = 0U;
    messages[ 1 ].segments = &data_segment;
    messages[ 1 ].segment_count = 1U;
    messages[ 1 ].flags = I2C_MESSAGE_READ;

    return i2c_transfer( &device,
                         messages,
                         2U,
                         eeprom->config.transfer_timeout_ms );
}

static StorageStatus eeprom_read( Storage *storage,
                                  uint64_t address,
                                  uint8_t *data,
                                  size_t length )
{
    I2cEeprom *eeprom = container_of( storage, I2cEeprom, base );

    while( length != 0U )
    {
        size_t chunk = limit_to_block( eeprom, address, length );
        StorageStatus status =
            eeprom_read_chunk( eeprom, address, data, chunk );

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

static StorageStatus eeprom_program_chunk( I2cEeprom *eeprom,
                                           uint64_t address,
                                           const uint8_t *data,
                                           size_t length )
{
    uint8_t word_address[ 2 ];
    I2cSegment segments[ 2 ];
    I2cMessage message;
    I2cDevice device = addressed_device( eeprom, address );
    StorageStatus status;

    encode_word_address( eeprom, address, word_address );
    segments[ 0 ].tx_data = word_address;
    segments[ 0 ].rx_data = 0;
    segments[ 0 ].length = eeprom->config.word_address_bytes;
    segments[ 1 ].tx_data = data;
    segments[ 1 ].rx_data = 0;
    segments[ 1 ].length = length;
    message.segments = segments;
    message.segment_count = 2U;
    message.flags = 0U;

    status = i2c_transfer( &device,
                           &message,
                           1U,
                           eeprom->config.transfer_timeout_ms );
    return ( status == STORAGE_OK ) ?
           wait_write_complete( eeprom, &device ) :
           status;
}

static StorageStatus eeprom_program( Storage *storage,
                                     uint64_t address,
                                     const uint8_t *data,
                                     size_t length )
{
    I2cEeprom *eeprom = container_of( storage, I2cEeprom, base );

    while( length != 0U )
    {
        uint32_t page_offset =
            ( uint32_t ) ( address % eeprom->config.page_size_bytes );
        size_t chunk =
            ( size_t ) ( eeprom->config.page_size_bytes - page_offset );
        StorageStatus status;

        if( chunk > length )
        {
            chunk = length;
        }
        chunk = limit_to_block( eeprom, address, chunk );

        status = eeprom_program_chunk( eeprom, address, data, chunk );
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

StorageStatus i2c_eeprom_construct( I2cEeprom *eeprom,
                                    const I2cEepromConfig *config )
{
    uint64_t maximum_capacity;

    if( ( eeprom == 0 ) ||
        ( config == 0 ) ||
        ( config->device.bus == 0 ) ||
        !hw_time_is_valid( &config->time ) ||
        ( config->capacity_bytes == 0U ) ||
        ( config->page_size_bytes == 0U ) ||
        ( config->page_size_bytes > config->capacity_bytes ) ||
        ( config->transfer_timeout_ms == 0U ) ||
        ( config->write_timeout_ms == 0U ) ||
        ( config->word_address_bytes < 1U ) ||
        ( config->word_address_bytes > 2U ) ||
        ( config->block_address_bits > 3U ) ||
        ( config->device.address > 0x7FU ) )
    {
        return STORAGE_INVALID_ARGUMENT;
    }

    maximum_capacity =
        ( 1ULL << ( config->word_address_bytes * 8U ) ) *
        ( 1ULL << config->block_address_bits );
    if( config->capacity_bytes > maximum_capacity )
    {
        return STORAGE_BAD_CONFIG;
    }

    memset( eeprom, 0, sizeof( *eeprom ) );
    eeprom->config = *config;
    storage_base_construct( &eeprom->base, config->name, &eeprom_ops );
    return STORAGE_OK;
}

Storage *i2c_eeprom_as_storage( I2cEeprom *eeprom )
{
    return ( eeprom != 0 ) ? &eeprom->base : 0;
}
