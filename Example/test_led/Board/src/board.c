#include "board.h"

#include <stdint.h>

#include "led_gpio.h"
#include "main.h"

LedBase *g_board_led;

static LedGpio board_led_obj;

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

void board_init( void )
{
    led_gpio_init( &board_led_obj, &board_led_config );
    g_board_led = led_gpio_as_base( &board_led_obj );
}
