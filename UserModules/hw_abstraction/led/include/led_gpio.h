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

/*
 * GPIO-backed LED object.
 *
 * Fields:
 *   base         Base LED object. Must be the first member.
 *   write        Board-specific GPIO write callback.
 *   ctx          Board-specific GPIO context passed to write.
 *   pin          Pin identifier passed to write.
 *   active_level Output level that turns the LED on: 0 or 1.
 */
typedef struct
{
    LedBase base;
    LedGpioWriteFn write;
    void *ctx;
    uint32_t pin;
    uint8_t active_level;
} LedGpio;

/*
 * Initialize a GPIO-backed LED and set it to off.
 *
 * Parameters:
 *   me           GPIO LED object to initialize. Must not be 0.
 *   name         Debug name. May be 0.
 *   write        GPIO write callback. Must not be 0.
 *   ctx          Board-specific GPIO context passed to write.
 *   pin          Pin identifier passed to write.
 *   active_level Output level that turns the LED on: 0 or 1.
 */
void led_gpio_init( LedGpio *me,
                    const char *name,
                    LedGpioWriteFn write,
                    void *ctx,
                    uint32_t pin,
                    uint8_t active_level );

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
