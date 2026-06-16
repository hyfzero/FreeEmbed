#ifndef TEST_LED_BOARD_H
#define TEST_LED_BOARD_H

#include "led_base.h"
#include "storage.h"

#ifdef __cplusplus
extern "C" {
#endif

extern LedBase *g_board_led;
extern Storage *g_board_flash;

void board_init( void );
StorageStatus board_flash_init( void );

#ifdef __cplusplus
}
#endif

#endif /* TEST_LED_BOARD_H */
