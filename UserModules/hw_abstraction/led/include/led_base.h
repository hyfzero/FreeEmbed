#ifndef USERMODULES_LED_BASE_H
#define USERMODULES_LED_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LedBase LedBase;

/*
 * LED operation table.
 *
 * Fields:
 *   on             Required. Turns the LED on.
 *   off            Required. Turns the LED off.
 *   set_brightness Optional. Sets brightness; use 0 when unsupported.
 */
typedef struct
{
    void ( *on )( LedBase *me );
    void ( *off )( LedBase *me );
    void ( *set_brightness )( LedBase *me, int val );
} LedOps;

/*
 * Base LED object.
 *
 * Fields:
 *   name  Debug name. May be 0.
 *   state Current abstract state: 1 on, 0 off, -1 unknown.
 *   ops   Operation table supplied by the concrete implementation.
 */
struct LedBase
{
    const char *name;
    int state;
    const LedOps *ops;
};

/*
 * Initialize common LED base fields.
 *
 * Parameters:
 *   me   LED base object to initialize. Must not be 0.
 *   name Debug name. May be 0.
 */
void led_base_init( LedBase *me, const char *name );

/*
 * Turn the LED on.
 *
 * Parameters:
 *   me LED base object. Must have valid ops->on.
 */
void led_on( LedBase *me );

/*
 * Turn the LED off.
 *
 * Parameters:
 *   me LED base object. Must have valid ops->off.
 */
void led_off( LedBase *me );

/*
 * Set LED brightness when supported.
 *
 * Parameters:
 *   me  LED base object. Must not be 0.
 *   val Brightness value passed to the concrete implementation.
 */
void led_set_brightness( LedBase *me, int val );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_LED_BASE_H */
