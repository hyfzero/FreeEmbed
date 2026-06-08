#ifndef USERMODULES_LED_GPIO_H
#define USERMODULES_LED_GPIO_H

#include <stdint.h>

#include "led_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GPIO write callback supplied by the board layer.
 *
 * Parameters:
 *   ctx   Board-specific GPIO context.
 *   pin   Pin identifier bound to this LED.
 *   level Output level: 0 or 1.
 */
typedef void ( *LedGpioWriteFn )( void *ctx, uint32_t pin, int level );

#define LED_GPIO_ACTIVE_LOW    0U
#define LED_GPIO_ACTIVE_HIGH   1U

#define LED_GPIO_LINE(write_fn, context, pin_id, active) \
    { ( write_fn ), ( context ), ( pin_id ), ( uint8_t ) ( ( active ) ? 1U : 0U ) }

#define LED_GPIO_CONFIG(name_str, write_fn, context, pin_id, active) \
    { ( name_str ), LED_GPIO_LINE( ( write_fn ), ( context ), ( pin_id ), ( active ) ) }

/*
 * GPIO line descriptor.
 *
 * Fields:
 *   write        Board-specific GPIO write callback.
 *   ctx          Board-specific GPIO context passed to write.
 *   pin          Pin identifier passed to write.
 *   active_level Output level that turns the LED on: 0 or 1.
 */
typedef struct
{
    LedGpioWriteFn write;
    void *ctx;
    uint32_t pin;
    uint8_t active_level;
} LedGpioLine;

/*
 * GPIO LED configuration.
 *
 * Fields:
 *   name Debug name. May be 0.
 *   line GPIO line descriptor used by this LED.
 */
typedef struct
{
    const char *name;
    LedGpioLine line;
} LedGpioConfig;

/*
 * GPIO-backed LED object.
 *
 * Fields:
 *   base         Base LED object. Must be the first member.
 *   line         Copied GPIO line descriptor.
 */
typedef struct
{
    LedBase base;
    LedGpioLine line;
} LedGpio;

/*
 * Initialize a GPIO-backed LED and set it to off.
 *
 * Parameters:
 *   me     GPIO LED object to initialize. Must not be 0.
 *   config GPIO LED configuration. Must not be 0 and must contain write.
 */
void led_gpio_init( LedGpio *me, const LedGpioConfig *config );

/*
 * Return the base LED view of a GPIO LED object.
 *
 * Parameters:
 *   me GPIO LED object. Must not be 0.
 */
LedBase *led_gpio_as_base( LedGpio *me );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_LED_GPIO_H */
