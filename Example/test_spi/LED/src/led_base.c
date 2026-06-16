#include "led_base.h"

#include <assert.h>

void led_base_init( LedBase *me, const char *name )
{
    assert( me != 0 );

    me->name = name;
    me->state = -1;
    me->ops = 0;
}

void led_on( LedBase *me )
{
    assert( me != 0 );
    assert( me->ops != 0 );
    assert( me->ops->on != 0 );

    me->ops->on( me );
    me->state = 1;
}

void led_off( LedBase *me )
{
    assert( me != 0 );
    assert( me->ops != 0 );
    assert( me->ops->off != 0 );

    me->ops->off( me );
    me->state = 0;
}

void led_set_brightness( LedBase *me, int val )
{
    assert( me != 0 );

    if( ( me->ops != 0 ) && ( me->ops->set_brightness != 0 ) )
    {
        me->ops->set_brightness( me, val );
    }
}
