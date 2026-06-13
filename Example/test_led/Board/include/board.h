#ifndef TEST_LED_BOARD_H
#define TEST_LED_BOARD_H

#include "led_base.h"

#ifdef __cplusplus
extern "C" {
#endif

extern LedBase *g_board_led;

void board_init( void );

#ifdef __cplusplus
}
#endif

#endif /* TEST_LED_BOARD_H */
