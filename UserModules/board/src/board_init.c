#include "board_init.h"

#include <stdint.h>

#include "led_gpio.h"

/*
 * Example GPIO port context.
 *
 * Fields:
 *   port_name  Debug name of the example port.
 *   last_pin   Last pin written by example_gpio_write.
 *   last_level Last level written by example_gpio_write.
 */
typedef struct
{
    const char *port_name;
    uint32_t last_pin;
    int last_level;
} ExampleGpioPort;

/* Global LED handles exported to the application layer. */
LedBase *g_led_alarm;
LedBase *g_led_status;
LedBase *g_led_network;

/* Static concrete LED objects owned by the board layer. */
static LedGpio alarm_obj;
static LedGpio status_obj;
static LedGpio network_obj;

static ExampleGpioPort alarm_port = { "ALARM_PORT", 0U, 0 };
static ExampleGpioPort status_port = { "STATUS_PORT", 0U, 0 };
static ExampleGpioPort network_port = { "NETWORK_PORT", 0U, 0 };

/*
 * Example GPIO write callback.
 *
 * Parameters:
 *   ctx   Pointer to an ExampleGpioPort object.
 *   pin   Pin identifier to store.
 *   level Output level to store: 0 or 1.
 */
static void example_gpio_write( void *ctx, uint32_t pin, int level )
{
    ExampleGpioPort *port = ( ExampleGpioPort * ) ctx;

    if( port != 0 )
    {
        port->last_pin = pin;
        port->last_level = level ? 1 : 0;
    }
}

void board_init( void )
{
    /* Bind logical LED names to concrete GPIO resources. */
    led_gpio_init( &alarm_obj, "alarm", example_gpio_write, &alarm_port, 0U, 1U );
    led_gpio_init( &status_obj, "status", example_gpio_write, &status_port, 1U, 1U );
    led_gpio_init( &network_obj, "network", example_gpio_write, &network_port, 2U, 0U );

    /* Export concrete objects as LedBase handles. */
    g_led_alarm = led_gpio_as_base( &alarm_obj );
    g_led_status = led_gpio_as_base( &status_obj );
    g_led_network = led_gpio_as_base( &network_obj );
}
