#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "i2c_eeprom.h"
#include "spi_nor.h"

#define ARRAY_SIZE(array)    ( sizeof( array ) / sizeof( ( array )[ 0 ] ) )

static int failures;

#define CHECK(condition) \
    do \
    { \
        if( !( condition ) ) \
        { \
            printf( "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition ); \
            failures++; \
            return; \
        } \
    } while( 0 )

typedef struct
{
    uint32_t now_ms;
} FakeTime;

static uint32_t fake_get_time_ms( void *ctx )
{
    return ( ( FakeTime * ) ctx )->now_ms;
}

static void fake_delay_ms( void *ctx, uint32_t delay_ms )
{
    ( ( FakeTime * ) ctx )->now_ms += delay_ms;
}

typedef struct
{
    uint8_t jedec_id[ 3 ];
    uint8_t address_bytes;
    uint8_t busy_forever;
    uint32_t busy_reads;
    uint32_t transaction_count;
    uint32_t two_segment_transactions;
    uint32_t legacy_id_probe_count;
    uint64_t legacy_id_address;
    uint32_t status_write_count;
    uint8_t status_write_values[ 8 ];
    uint32_t page_program_count;
    uint32_t erase_count;
    uint8_t opcodes[ 64 ];
    uint32_t opcode_count;
    uint64_t program_addresses[ 8 ];
    size_t program_lengths[ 8 ];
    uint8_t erase_opcodes[ 8 ];
    uint64_t erase_addresses[ 8 ];
} FakeSpi;

static uint64_t decode_address( const uint8_t *data, uint8_t address_bytes )
{
    uint64_t address = 0U;
    uint8_t index;

    for( index = 0U; index < address_bytes; index++ )
    {
        address = ( address << 8 ) | data[ index ];
    }

    return address;
}

static HwStatus fake_spi_transfer( void *ctx,
                                   const SpiDevice *device,
                                   const SpiSegment *segments,
                                   size_t segment_count,
                                   uint32_t timeout_ms )
{
    FakeSpi *fake = ( FakeSpi * ) ctx;
    uint8_t opcode;

    ( void ) device;
    ( void ) timeout_ms;
    fake->transaction_count++;
    if( segment_count == 2U )
    {
        fake->two_segment_transactions++;
    }

    if( ( segments[ 0 ].tx_data == 0 ) || ( segments[ 0 ].length == 0U ) )
    {
        return HW_STATUS_IO_ERROR;
    }

    opcode = segments[ 0 ].tx_data[ 0 ];
    if( fake->opcode_count < ARRAY_SIZE( fake->opcodes ) )
    {
        fake->opcodes[ fake->opcode_count++ ] = opcode;
    }

    if( opcode == 0x9FU )
    {
        memcpy( segments[ 1 ].rx_data, fake->jedec_id, 3U );
    }
    else if( opcode == 0x90U )
    {
        fake->legacy_id_probe_count++;
        fake->legacy_id_address =
            decode_address( &segments[ 0 ].tx_data[ 1 ], 3U );
        memcpy( segments[ 1 ].rx_data, fake->jedec_id, 2U );
    }
    else if( opcode == 0x05U )
    {
        if( fake->busy_forever != 0U )
        {
            segments[ 1 ].rx_data[ 0 ] = 0x01U;
        }
        else if( fake->busy_reads != 0U )
        {
            segments[ 1 ].rx_data[ 0 ] = 0x01U;
            fake->busy_reads--;
        }
        else
        {
            segments[ 1 ].rx_data[ 0 ] = 0U;
        }
    }
    else if( opcode == 0x01U )
    {
        uint32_t index = fake->status_write_count++;

        if( ( index < ARRAY_SIZE( fake->status_write_values ) ) &&
            ( segments[ 0 ].length >= 2U ) )
        {
            fake->status_write_values[ index ] = segments[ 0 ].tx_data[ 1 ];
        }
    }
    else if( opcode == 0x03U )
    {
        size_t index;

        for( index = 0U; index < segments[ 1 ].length; index++ )
        {
            segments[ 1 ].rx_data[ index ] = ( uint8_t ) index;
        }
    }
    else if( opcode == 0x02U )
    {
        uint32_t index = fake->page_program_count++;

        if( index < ARRAY_SIZE( fake->program_addresses ) )
        {
            fake->program_addresses[ index ] =
                decode_address( &segments[ 0 ].tx_data[ 1 ],
                                fake->address_bytes );
            fake->program_lengths[ index ] = segments[ 1 ].length;
        }
    }
    else if( ( opcode == 0x20U ) ||
             ( opcode == 0x52U ) ||
             ( opcode == 0xD8U ) )
    {
        uint32_t index = fake->erase_count++;

        if( index < ARRAY_SIZE( fake->erase_opcodes ) )
        {
            fake->erase_opcodes[ index ] = opcode;
            fake->erase_addresses[ index ] =
                decode_address( &segments[ 0 ].tx_data[ 1 ],
                                fake->address_bytes );
        }
    }

    return HW_STATUS_OK;
}

static void setup_spi_nor( SpiNor *nor,
                           FakeSpi *fake_spi,
                           FakeTime *fake_time )
{
    static const SpiBusOps bus_ops = { fake_spi_transfer };
    static SpiBus bus;
    SpiNorConfig config;

    bus.ops = &bus_ops;
    bus.ctx = fake_spi;
    memset( &config, 0, sizeof( config ) );
    config.name = "test_nor";
    config.device.bus = &bus;
    config.device.chip_select = 3U;
    config.device.max_frequency_hz = 10000000U;
    config.device.mode = 0U;
    config.time.get_time_ms = fake_get_time_ms;
    config.time.delay_ms = fake_delay_ms;
    config.time.ctx = fake_time;
    config.transfer_timeout_ms = 10U;

    CHECK( spi_nor_construct( nor, &config ) == STORAGE_OK );
}

static void test_spi_nor_init_and_read_transaction( void )
{
    SpiNor nor;
    FakeSpi fake_spi;
    FakeTime fake_time = { 0U };
    StorageInfo info;
    uint8_t data[ 4 ];

    memset( &fake_spi, 0, sizeof( fake_spi ) );
    fake_spi.jedec_id[ 0 ] = 0xEFU;
    fake_spi.jedec_id[ 1 ] = 0x40U;
    fake_spi.jedec_id[ 2 ] = 0x15U;
    fake_spi.address_bytes = 3U;
    setup_spi_nor( &nor, &fake_spi, &fake_time );

    CHECK( storage_init( spi_nor_as_storage( &nor ) ) == STORAGE_OK );
    CHECK( storage_get_info( &nor.base, &info ) == STORAGE_OK );
    CHECK( info.capacity_bytes == 2ULL * 1024ULL * 1024ULL );
    CHECK( info.page_size_bytes == 256U );
    CHECK( info.erase_size_bytes == 4096U );
    CHECK( ( info.capabilities & STORAGE_CAP_REQUIRES_ERASE ) != 0U );
    CHECK( storage_read( &nor.base, 0x1234U, data, sizeof( data ) ) ==
           STORAGE_OK );
    CHECK( data[ 0 ] == 0U && data[ 3 ] == 3U );
    CHECK( fake_spi.two_segment_transactions == 2U );
}

static void test_spi_nor_page_split_and_erase_selection( void )
{
    SpiNor nor;
    FakeSpi fake_spi;
    FakeTime fake_time = { 0U };
    uint8_t data[ 20 ] = { 0U };

    memset( &fake_spi, 0, sizeof( fake_spi ) );
    fake_spi.jedec_id[ 0 ] = 0xEFU;
    fake_spi.jedec_id[ 1 ] = 0x40U;
    fake_spi.jedec_id[ 2 ] = 0x15U;
    fake_spi.address_bytes = 3U;
    setup_spi_nor( &nor, &fake_spi, &fake_time );
    CHECK( storage_init( &nor.base ) == STORAGE_OK );

    CHECK( storage_program( &nor.base, 250U, data, sizeof( data ) ) ==
           STORAGE_OK );
    CHECK( fake_spi.page_program_count == 2U );
    CHECK( fake_spi.program_addresses[ 0 ] == 250U );
    CHECK( fake_spi.program_lengths[ 0 ] == 6U );
    CHECK( fake_spi.program_addresses[ 1 ] == 256U );
    CHECK( fake_spi.program_lengths[ 1 ] == 14U );

    CHECK( storage_erase( &nor.base, 0U, 64U * 1024U ) == STORAGE_OK );
    CHECK( fake_spi.erase_count == 1U );
    CHECK( fake_spi.erase_opcodes[ 0 ] == 0xD8U );
    CHECK( storage_erase( &nor.base, 1U, 4096U ) == STORAGE_UNALIGNED );
}

static void test_spi_nor_timeout_wraparound( void )
{
    SpiNor nor;
    FakeSpi fake_spi;
    FakeTime fake_time = { UINT32_MAX - 10U };
    uint8_t value = 0xAAU;

    memset( &fake_spi, 0, sizeof( fake_spi ) );
    fake_spi.jedec_id[ 0 ] = 0xEFU;
    fake_spi.jedec_id[ 1 ] = 0x40U;
    fake_spi.jedec_id[ 2 ] = 0x15U;
    fake_spi.address_bytes = 3U;
    setup_spi_nor( &nor, &fake_spi, &fake_time );
    CHECK( storage_init( &nor.base ) == STORAGE_OK );
    fake_spi.busy_forever = 1U;

    CHECK( storage_program( &nor.base, 0U, &value, 1U ) ==
           STORAGE_TIMEOUT );
    CHECK( fake_time.now_ms >= 9U );
}

static void test_spi_nor_sst25vf010a_init_unlock_and_byte_program( void )
{
    SpiNor nor;
    FakeSpi fake_spi;
    FakeTime fake_time = { 0U };
    StorageInfo info;
    uint8_t data[ 3 ] = { 0x12U, 0x34U, 0x56U };

    memset( &fake_spi, 0, sizeof( fake_spi ) );
    fake_spi.jedec_id[ 0 ] = 0xBFU;
    fake_spi.jedec_id[ 1 ] = 0x49U;
    fake_spi.address_bytes = 3U;
    setup_spi_nor( &nor, &fake_spi, &fake_time );

    CHECK( storage_init( &nor.base ) == STORAGE_OK );
    CHECK( storage_get_info( &nor.base, &info ) == STORAGE_OK );
    CHECK( info.capacity_bytes == 128ULL * 1024ULL );
    CHECK( info.page_size_bytes == 1U );
    CHECK( info.erase_size_bytes == 4096U );
    CHECK( fake_spi.legacy_id_probe_count == 1U );
    CHECK( fake_spi.legacy_id_address == 0U );
    CHECK( fake_spi.status_write_count == 1U );
    CHECK( fake_spi.status_write_values[ 0 ] == 0U );
    CHECK( fake_spi.opcodes[ 1 ] == 0x90U );
    CHECK( fake_spi.opcodes[ 2 ] == 0x50U );
    CHECK( fake_spi.opcodes[ 3 ] == 0x01U );

    CHECK( storage_program( &nor.base, 5U, data, sizeof( data ) ) ==
           STORAGE_OK );
    CHECK( fake_spi.page_program_count == 3U );
    CHECK( fake_spi.program_addresses[ 0 ] == 5U );
    CHECK( fake_spi.program_addresses[ 1 ] == 6U );
    CHECK( fake_spi.program_addresses[ 2 ] == 7U );
    CHECK( fake_spi.program_lengths[ 0 ] == 1U );
    CHECK( fake_spi.program_lengths[ 1 ] == 1U );
    CHECK( fake_spi.program_lengths[ 2 ] == 1U );
    CHECK( fake_spi.opcodes[ 5 ] == 0x06U );
    CHECK( fake_spi.opcodes[ 6 ] == 0x02U );
    CHECK( fake_spi.opcodes[ 7 ] == 0x05U );
    CHECK( fake_spi.opcodes[ 8 ] == 0x06U );
    CHECK( fake_spi.opcodes[ 9 ] == 0x02U );
    CHECK( fake_spi.opcodes[ 10 ] == 0x05U );
    CHECK( fake_spi.opcodes[ 11 ] == 0x06U );
    CHECK( fake_spi.opcodes[ 12 ] == 0x02U );
    CHECK( fake_spi.opcodes[ 13 ] == 0x05U );
}

static void test_spi_nor_sst25vf010a_erase_selection( void )
{
    SpiNor nor;
    FakeSpi fake_spi;
    FakeTime fake_time = { 0U };

    memset( &fake_spi, 0, sizeof( fake_spi ) );
    fake_spi.jedec_id[ 0 ] = 0xBFU;
    fake_spi.jedec_id[ 1 ] = 0x49U;
    fake_spi.address_bytes = 3U;
    setup_spi_nor( &nor, &fake_spi, &fake_time );
    CHECK( storage_init( &nor.base ) == STORAGE_OK );

    CHECK( storage_erase( &nor.base, 0U, 32U * 1024U ) == STORAGE_OK );
    CHECK( fake_spi.erase_count == 1U );
    CHECK( fake_spi.erase_opcodes[ 0 ] == 0x52U );
    CHECK( storage_erase( &nor.base, 1U, 4096U ) == STORAGE_UNALIGNED );
}

typedef struct
{
    uint32_t transfer_count;
    uint32_t probe_count;
    uint32_t busy_probes;
    uint16_t addresses[ 16 ];
    size_t message_counts[ 16 ];
    size_t write_data_lengths[ 16 ];
    uint8_t word_addresses[ 16 ][ 2 ];
} FakeI2c;

static HwStatus fake_i2c_transfer( void *ctx,
                                   const I2cDevice *device,
                                   const I2cMessage *messages,
                                   size_t message_count,
                                   uint32_t timeout_ms )
{
    FakeI2c *fake = ( FakeI2c * ) ctx;
    uint32_t index = fake->transfer_count++;
    size_t byte_index;

    ( void ) timeout_ms;
    if( index < ARRAY_SIZE( fake->addresses ) )
    {
        fake->addresses[ index ] = device->address;
        fake->message_counts[ index ] = message_count;
        fake->word_addresses[ index ][ 0 ] =
            messages[ 0 ].segments[ 0 ].tx_data[ 0 ];
        if( messages[ 0 ].segments[ 0 ].length == 2U )
        {
            fake->word_addresses[ index ][ 1 ] =
                messages[ 0 ].segments[ 0 ].tx_data[ 1 ];
        }
        if( ( message_count == 1U ) &&
            ( messages[ 0 ].segment_count == 2U ) )
        {
            fake->write_data_lengths[ index ] =
                messages[ 0 ].segments[ 1 ].length;
        }
    }

    if( ( message_count == 2U ) &&
        ( messages[ 1 ].flags & I2C_MESSAGE_READ ) != 0U )
    {
        I2cSegment segment = messages[ 1 ].segments[ 0 ];

        for( byte_index = 0U; byte_index < segment.length; byte_index++ )
        {
            segment.rx_data[ byte_index ] = ( uint8_t ) byte_index;
        }
    }

    return HW_STATUS_OK;
}

static HwStatus fake_i2c_probe( void *ctx,
                                const I2cDevice *device,
                                uint32_t timeout_ms )
{
    FakeI2c *fake = ( FakeI2c * ) ctx;

    ( void ) device;
    ( void ) timeout_ms;
    fake->probe_count++;
    if( fake->busy_probes != 0U )
    {
        fake->busy_probes--;
        return HW_STATUS_BUSY;
    }

    return HW_STATUS_OK;
}

static void setup_eeprom( I2cEeprom *eeprom,
                          FakeI2c *fake_i2c,
                          FakeTime *fake_time,
                          uint64_t capacity,
                          uint32_t page_size,
                          uint8_t word_address_bytes,
                          uint8_t block_address_bits )
{
    static const I2cBusOps bus_ops =
    {
        fake_i2c_transfer,
        fake_i2c_probe
    };
    static I2cBus bus;
    I2cEepromConfig config;

    bus.ops = &bus_ops;
    bus.ctx = fake_i2c;
    memset( &config, 0, sizeof( config ) );
    config.name = "test_eeprom";
    config.device.bus = &bus;
    config.device.address = 0x50U;
    config.device.max_frequency_hz = 400000U;
    config.time.get_time_ms = fake_get_time_ms;
    config.time.delay_ms = fake_delay_ms;
    config.time.ctx = fake_time;
    config.capacity_bytes = capacity;
    config.page_size_bytes = page_size;
    config.transfer_timeout_ms = 5U;
    config.write_timeout_ms = 10U;
    config.word_address_bytes = word_address_bytes;
    config.block_address_bits = block_address_bits;

    CHECK( i2c_eeprom_construct( eeprom, &config ) == STORAGE_OK );
}

static void test_eeprom_block_address_and_repeated_start( void )
{
    I2cEeprom eeprom;
    FakeI2c fake_i2c;
    FakeTime fake_time = { 0U };
    uint8_t data[ 20 ];

    memset( &fake_i2c, 0, sizeof( fake_i2c ) );
    setup_eeprom( &eeprom, &fake_i2c, &fake_time, 2048U, 16U, 1U, 3U );
    CHECK( storage_init( &eeprom.base ) == STORAGE_OK );
    CHECK( storage_read( &eeprom.base, 250U, data, sizeof( data ) ) ==
           STORAGE_OK );
    CHECK( fake_i2c.transfer_count == 2U );
    CHECK( fake_i2c.addresses[ 0 ] == 0x50U );
    CHECK( fake_i2c.addresses[ 1 ] == 0x51U );
    CHECK( fake_i2c.message_counts[ 0 ] == 2U );
    CHECK( fake_i2c.message_counts[ 1 ] == 2U );
    CHECK( fake_i2c.word_addresses[ 0 ][ 0 ] == 250U );
    CHECK( fake_i2c.word_addresses[ 1 ][ 0 ] == 0U );
}

static void test_eeprom_page_split_and_ack_polling( void )
{
    I2cEeprom eeprom;
    FakeI2c fake_i2c;
    FakeTime fake_time = { 0U };
    uint8_t data[ 20 ] = { 0U };

    memset( &fake_i2c, 0, sizeof( fake_i2c ) );
    setup_eeprom( &eeprom, &fake_i2c, &fake_time, 32768U, 16U, 2U, 0U );
    CHECK( storage_init( &eeprom.base ) == STORAGE_OK );
    fake_i2c.busy_probes = 2U;
    CHECK( storage_program( &eeprom.base, 14U, data, sizeof( data ) ) ==
           STORAGE_OK );
    CHECK( fake_i2c.transfer_count == 3U );
    CHECK( fake_i2c.write_data_lengths[ 0 ] == 2U );
    CHECK( fake_i2c.write_data_lengths[ 1 ] == 16U );
    CHECK( fake_i2c.write_data_lengths[ 2 ] == 2U );
    CHECK( fake_i2c.word_addresses[ 0 ][ 0 ] == 0U );
    CHECK( fake_i2c.word_addresses[ 0 ][ 1 ] == 14U );
    CHECK( fake_i2c.probe_count == 6U );
    CHECK( storage_erase( &eeprom.base, 0U, 16U ) ==
           STORAGE_UNSUPPORTED );
}

static void test_common_validation( void )
{
    SpiNor nor;
    FakeSpi fake_spi;
    FakeTime fake_time = { 0U };
    uint8_t byte = 0U;

    memset( &fake_spi, 0, sizeof( fake_spi ) );
    fake_spi.jedec_id[ 0 ] = 0xEFU;
    fake_spi.jedec_id[ 1 ] = 0x40U;
    fake_spi.jedec_id[ 2 ] = 0x15U;
    fake_spi.address_bytes = 3U;
    setup_spi_nor( &nor, &fake_spi, &fake_time );

    CHECK( storage_read( &nor.base, 0U, &byte, 1U ) ==
           STORAGE_NOT_INITIALIZED );
    CHECK( storage_init( &nor.base ) == STORAGE_OK );
    CHECK( storage_read( &nor.base,
                         nor.base.info.capacity_bytes,
                         &byte,
                         1U ) == STORAGE_OUT_OF_RANGE );
    CHECK( storage_program( &nor.base, 0U, 0, 1U ) ==
           STORAGE_INVALID_ARGUMENT );
    CHECK( storage_read( &nor.base, 0U, 0, 0U ) == STORAGE_OK );
}

int main( void )
{
    test_spi_nor_init_and_read_transaction();
    test_spi_nor_page_split_and_erase_selection();
    test_spi_nor_timeout_wraparound();
    test_spi_nor_sst25vf010a_init_unlock_and_byte_program();
    test_spi_nor_sst25vf010a_erase_selection();
    test_eeprom_block_address_and_repeated_start();
    test_eeprom_page_split_and_ack_polling();
    test_common_validation();

    if( failures != 0 )
    {
        printf( "%d test(s) failed\n", failures );
        return 1;
    }

    printf( "all storage tests passed\n" );
    return 0;
}
