#ifndef TEST_LED_LED_BASE_H
#define TEST_LED_LED_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LedBase LedBase;

typedef struct
{
    void ( *on )( LedBase *me );
    void ( *off )( LedBase *me );
    void ( *set_brightness )( LedBase *me, int val );
} LedOps;

struct LedBase
{
    const char *name;
    int state;
    const LedOps *ops;
};

void led_base_init( LedBase *me, const char *name );
void led_on( LedBase *me );
void led_off( LedBase *me );
void led_set_brightness( LedBase *me, int val );

#ifdef __cplusplus
}
#endif

#endif /* TEST_LED_LED_BASE_H */
