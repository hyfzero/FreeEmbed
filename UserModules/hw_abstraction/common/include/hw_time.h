#ifndef USERMODULES_HW_TIME_H
#define USERMODULES_HW_TIME_H

#include <stdint.h>

typedef uint32_t ( *HwGetTimeMsFn )( void *ctx );
typedef void ( *HwDelayMsFn )( void *ctx, uint32_t delay_ms );

/*
 * Platform-neutral time source used by synchronous drivers.
 * Unsigned subtraction makes elapsed-time checks safe across tick wraparound.
 */
typedef struct
{
    HwGetTimeMsFn get_time_ms;
    HwDelayMsFn delay_ms;
    void *ctx;
} HwTime;

static inline uint32_t hw_time_elapsed_ms( uint32_t start_ms, uint32_t now_ms )
{
    return now_ms - start_ms;
}

static inline int hw_time_is_valid( const HwTime *time )
{
    return ( time != 0 ) &&
           ( time->get_time_ms != 0 ) &&
           ( time->delay_ms != 0 );
}

#endif /* USERMODULES_HW_TIME_H */
