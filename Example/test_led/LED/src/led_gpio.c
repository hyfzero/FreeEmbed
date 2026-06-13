#include "led_gpio.h"

#include <assert.h>

#include "container_of.h"

static void gpio_on( LedBase *base )
{
    LedGpio *me = container_of( base, LedGpio, base );

    me->line.write( me->line.ctx, me->line.pin, me->line.active_level ? 1 : 0 );
}

static void gpio_off( LedBase *base )
{
    LedGpio *me = container_of( base, LedGpio, base );

    me->line.write( me->line.ctx, me->line.pin, me->line.active_level ? 0 : 1 );
}

static const LedOps gpio_ops =
{
    gpio_on,
    gpio_off,
    0
};

void led_gpio_init( LedGpio *me, const LedGpioConfig *config )
{
    assert( me != 0 );
    assert( config != 0 );
    assert( config->line.write != 0 );

    led_base_init( &me->base, config->name );
    me->line = config->line;
    me->line.active_level = config->line.active_level ? 1U : 0U;
    me->base.ops = &gpio_ops;

    gpio_off( &me->base );
}

LedBase *led_gpio_as_base( LedGpio *me )
{
    assert( me != 0 );

    return &me->base;
}
