#ifndef USERMODULES_HW_STATUS_H
#define USERMODULES_HW_STATUS_H

/*
 * Common status values shared by transports and reusable device drivers.
 * Platform adapters must translate vendor-specific return values to this enum.
 */
typedef enum
{
    HW_STATUS_OK = 0,
    HW_STATUS_INVALID_ARGUMENT = -1,
    HW_STATUS_NOT_INITIALIZED = -2,
    HW_STATUS_OUT_OF_RANGE = -3,
    HW_STATUS_UNALIGNED = -4,
    HW_STATUS_UNSUPPORTED = -5,
    HW_STATUS_IO_ERROR = -6,
    HW_STATUS_TIMEOUT = -7,
    HW_STATUS_NOT_FOUND = -8,
    HW_STATUS_BUSY = -9,
    HW_STATUS_BAD_CONFIG = -10
} HwStatus;

#endif /* USERMODULES_HW_STATUS_H */
