#ifndef USERMODULES_BOARD_INIT_H
#define USERMODULES_BOARD_INIT_H

#include "led_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Application-visible LED handles.
 *
 * Values:
 *   g_led_alarm   Alarm LED handle.
 *   g_led_status  Status LED handle.
 *   g_led_network Network LED handle.
 */
extern LedBase *g_led_alarm;
extern LedBase *g_led_status;
extern LedBase *g_led_network;

/* Initialize board-level LED objects and bind the global LED handles. */
void board_init( void );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_BOARD_INIT_H */
