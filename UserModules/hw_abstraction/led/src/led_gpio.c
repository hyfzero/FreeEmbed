#include "led_gpio.h"

#include <assert.h>

#include "container_of.h"

static void gpio_on( LedBase *base )
{
    /* base: LED base view of a LedGpio object. */
    LedGpio *me = container_of( base, LedGpio, base );

    me->write( me->ctx, me->pin, me->active_level ? 1 : 0 );
}

static void gpio_off( LedBase *base )
{
    /* base: LED base view of a LedGpio object. */
    LedGpio *me = container_of( base, LedGpio, base );

    me->write( me->ctx, me->pin, me->active_level ? 0 : 1 );
}

/* GPIO LEDs only provide on/off; set_brightness is unsupported. */
static const LedOps gpio_ops =
{
    gpio_on,
    gpio_off,
    0
};

void led_gpio_init( LedGpio *me,
                    const char *name,
                    LedGpioWriteFn write,
                    void *ctx,
                    uint32_t pin,
                    uint8_t active_level )
{
    /* me/name/write/ctx/pin/active_level: see led_gpio.h. */
    assert( me != 0 );
    assert( write != 0 );

    led_base_init( &me->base, name );
    me->write = write;
    me->ctx = ctx;
    me->pin = pin;
    me->active_level = active_level ? 1U : 0U;
    me->base.ops = &gpio_ops;

    /* Start from a known off state after initialization. */
    gpio_off( &me->base );
}

LedBase *led_gpio_as_base( LedGpio *me )
{
    assert( me != 0 );

    return &me->base;
}
