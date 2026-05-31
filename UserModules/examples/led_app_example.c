#include "board_init.h"
#include "led_base.h"

/*
 * Run one application-level LED example.
 *
 * Parameters:
 *   None.
 */
void led_app_example_run_once( void )
{
    board_init();

    led_on( g_led_alarm );
    led_off( g_led_status );
    led_set_brightness( g_led_network, 50 );
}
