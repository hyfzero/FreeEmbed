#include "board.h"

#include <stdint.h>

#include "led_gpio.h"
#include "main.h"
#include "spi_nor.h"
#include "spi_transport.h"

LedBase *g_board_led;
Storage *g_board_flash;

static LedGpio board_led_obj;
static SPI_HandleTypeDef board_spi1_handle;
static SpiNor board_flash_nor;

typedef struct
{
    SPI_HandleTypeDef *handle;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} BoardSpiContext;

static BoardSpiContext board_spi_context =
{
    &board_spi1_handle,
    GPIOA,
    GPIO_PIN_4
};

static void led_hal_write( void *ctx, uint32_t pin, int level )
{
    HAL_GPIO_WritePin( ( GPIO_TypeDef * ) ctx,
                       ( uint16_t ) pin,
                       level ? GPIO_PIN_SET : GPIO_PIN_RESET );
}

static const LedGpioConfig board_led_config =
    LED_GPIO_CONFIG( "board_led",
                     led_hal_write,
                     GPIOB,
                     GPIO_PIN_5,
                     LED_GPIO_ACTIVE_HIGH );

static uint32_t board_get_time_ms( void *ctx )
{
    ( void ) ctx;
    return HAL_GetTick();
}

static void board_delay_ms( void *ctx, uint32_t delay_ms )
{
    ( void ) ctx;
    HAL_Delay( delay_ms );
}

static HwStatus board_hal_to_hw_status( HAL_StatusTypeDef status )
{
    if( status == HAL_OK )
    {
        return HW_STATUS_OK;
    }

    if( status == HAL_TIMEOUT )
    {
        return HW_STATUS_TIMEOUT;
    }

    if( status == HAL_BUSY )
    {
        return HW_STATUS_BUSY;
    }

    return HW_STATUS_IO_ERROR;
}

static HwStatus board_spi_transfer_fill( SPI_HandleTypeDef *handle,
                                         uint8_t fill_byte,
                                         uint8_t *rx_data,
                                         size_t length,
                                         uint32_t timeout_ms )
{
    size_t index;

    for( index = 0U; index < length; index++ )
    {
        uint8_t rx_byte = 0U;
        HAL_StatusTypeDef status =
            HAL_SPI_TransmitReceive( handle,
                                     &fill_byte,
                                     &rx_byte,
                                     1U,
                                     timeout_ms );

        if( status != HAL_OK )
        {
            return board_hal_to_hw_status( status );
        }

        if( rx_data != 0 )
        {
            rx_data[ index ] = rx_byte;
        }
    }

    return HW_STATUS_OK;
}

static HwStatus board_spi_transfer_chunked( SPI_HandleTypeDef *handle,
                                            const uint8_t *tx_data,
                                            uint8_t *rx_data,
                                            size_t length,
                                            uint32_t timeout_ms )
{
    while( length != 0U )
    {
        uint16_t chunk = ( length > 0xFFFFU ) ? 0xFFFFU :
                                                   ( uint16_t ) length;
        HAL_StatusTypeDef status;

        if( ( tx_data != 0 ) && ( rx_data != 0 ) )
        {
            status = HAL_SPI_TransmitReceive( handle,
                                              ( uint8_t * ) tx_data,
                                              rx_data,
                                              chunk,
                                              timeout_ms );
        }
        else if( tx_data != 0 )
        {
            status = HAL_SPI_Transmit( handle,
                                       ( uint8_t * ) tx_data,
                                       chunk,
                                       timeout_ms );
        }
        else
        {
            return HW_STATUS_INVALID_ARGUMENT;
        }

        if( status != HAL_OK )
        {
            return board_hal_to_hw_status( status );
        }

        if( tx_data != 0 )
        {
            tx_data += chunk;
        }

        if( rx_data != 0 )
        {
            rx_data += chunk;
        }

        length -= chunk;
    }

    return HW_STATUS_OK;
}

static HwStatus board_spi_transfer( void *ctx,
                                    const SpiDevice *device,
                                    const SpiSegment *segments,
                                    size_t segment_count,
                                    uint32_t timeout_ms )
{
    BoardSpiContext *spi_context = ( BoardSpiContext * ) ctx;
    size_t index;
    HwStatus result = HW_STATUS_OK;

    if( ( spi_context == 0 ) ||
        ( spi_context->handle == 0 ) ||
        ( device == 0 ) ||
        ( device->mode != 3U ) )
    {
        return HW_STATUS_BAD_CONFIG;
    }

    HAL_GPIO_WritePin( spi_context->cs_port,
                       spi_context->cs_pin,
                       GPIO_PIN_RESET );

    for( index = 0U; index < segment_count; index++ )
    {
        const SpiSegment *segment = &segments[ index ];

        if( segment->tx_data == 0 )
        {
            result = board_spi_transfer_fill( spi_context->handle,
                                              segment->fill_byte,
                                              segment->rx_data,
                                              segment->length,
                                              timeout_ms );
        }
        else
        {
            result = board_spi_transfer_chunked( spi_context->handle,
                                                 segment->tx_data,
                                                 segment->rx_data,
                                                 segment->length,
                                                 timeout_ms );
        }

        if( result != HW_STATUS_OK )
        {
            break;
        }
    }

    HAL_GPIO_WritePin( spi_context->cs_port,
                       spi_context->cs_pin,
                       GPIO_PIN_SET );
    return result;
}

static const SpiBusOps board_spi_ops =
{
    board_spi_transfer
};

static SpiBus board_spi_bus =
{
    &board_spi_ops,
    &board_spi_context
};

static StorageStatus board_spi1_init( void )
{
    board_spi1_handle.Instance = SPI1;
    board_spi1_handle.Init.Mode = SPI_MODE_MASTER;
    board_spi1_handle.Init.Direction = SPI_DIRECTION_2LINES;
    board_spi1_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    board_spi1_handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    board_spi1_handle.Init.CLKPhase = SPI_PHASE_2EDGE;
    board_spi1_handle.Init.NSS = SPI_NSS_SOFT;
    board_spi1_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    board_spi1_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    board_spi1_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    board_spi1_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    board_spi1_handle.Init.CRCPolynomial = 7U;

    if( HAL_SPI_Init( &board_spi1_handle ) != HAL_OK )
    {
        return STORAGE_IO_ERROR;
    }

    return STORAGE_OK;
}

void board_init( void )
{
    SpiNorConfig flash_config;

    led_gpio_init( &board_led_obj, &board_led_config );
    g_board_led = led_gpio_as_base( &board_led_obj );

    flash_config.name = "w25q64";
    flash_config.device.bus = &board_spi_bus;
    flash_config.device.chip_select = 0U;
    flash_config.device.max_frequency_hz = 18000000UL;
    flash_config.device.mode = 3U;
    flash_config.time.get_time_ms = board_get_time_ms;
    flash_config.time.delay_ms = board_delay_ms;
    flash_config.time.ctx = 0;
    flash_config.profiles = 0;
    flash_config.profile_count = 0U;
    flash_config.transfer_timeout_ms = 1000U;

    if( spi_nor_construct( &board_flash_nor, &flash_config ) == STORAGE_OK )
    {
        g_board_flash = spi_nor_as_storage( &board_flash_nor );
    }
    else
    {
        g_board_flash = 0;
    }
}

StorageStatus board_flash_init( void )
{
    StorageStatus status;

    if( g_board_flash == 0 )
    {
        return STORAGE_NOT_INITIALIZED;
    }

    status = board_spi1_init();
    if( status != STORAGE_OK )
    {
        return status;
    }

    return storage_init( g_board_flash );
}

void HAL_SPI_MspInit( SPI_HandleTypeDef *hspi )
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    if( hspi->Instance == SPI1 )
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_SPI1_CLK_ENABLE();

        HAL_GPIO_WritePin( GPIOA, GPIO_PIN_4, GPIO_PIN_SET );

        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init( GPIOA, &GPIO_InitStruct );

        GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init( GPIOA, &GPIO_InitStruct );

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init( GPIOA, &GPIO_InitStruct );
    }
}
