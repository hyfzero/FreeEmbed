#ifndef TEST_LED_LED_GPIO_H
#define TEST_LED_LED_GPIO_H

#include <stdint.h>

#include "led_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void ( *LedGpioWriteFn )( void *ctx, uint32_t pin, int level );

#define LED_GPIO_ACTIVE_LOW    0U
#define LED_GPIO_ACTIVE_HIGH   1U

#define LED_GPIO_LINE(write_fn, context, pin_id, active) \
    { ( write_fn ), ( context ), ( pin_id ), ( uint8_t ) ( ( active ) ? 1U : 0U ) }

#define LED_GPIO_CONFIG(name_str, write_fn, context, pin_id, active) \
    { ( name_str ), LED_GPIO_LINE( ( write_fn ), ( context ), ( pin_id ), ( active ) ) }

typedef struct
{
    LedGpioWriteFn write;
    void *ctx;
    uint32_t pin;
    uint8_t active_level;
} LedGpioLine;

typedef struct
{
    const char *name;
    LedGpioLine line;
} LedGpioConfig;

typedef struct
{
    LedBase base;
    LedGpioLine line;
} LedGpio;

void led_gpio_init( LedGpio *me, const LedGpioConfig *config );
LedBase *led_gpio_as_base( LedGpio *me );

#ifdef __cplusplus
}
#endif

#endif /* TEST_LED_LED_GPIO_H */
