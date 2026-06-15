#ifndef USERMODULES_I2C_TRANSPORT_H
#define USERMODULES_I2C_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "hw_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MESSAGE_READ    ( 1UL << 0 )

typedef struct I2cBus I2cBus;
typedef struct I2cDevice I2cDevice;

/*
 * Segments in one message are transferred continuously without a restart.
 * Multiple messages are separated by repeated starts and stopped at the end.
 */
typedef struct
{
    const uint8_t *tx_data;
    uint8_t *rx_data;
    size_t length;
} I2cSegment;

typedef struct
{
    const I2cSegment *segments;
    size_t segment_count;
    uint32_t flags;
} I2cMessage;

typedef HwStatus ( *I2cTransferFn )( void *ctx,
                                    const I2cDevice *device,
                                    const I2cMessage *messages,
                                    size_t message_count,
                                    uint32_t timeout_ms );
typedef HwStatus ( *I2cProbeFn )( void *ctx,
                                 const I2cDevice *device,
                                 uint32_t timeout_ms );

typedef struct
{
    I2cTransferFn transfer;
    I2cProbeFn probe;
} I2cBusOps;

struct I2cBus
{
    const I2cBusOps *ops;
    void *ctx;
};

struct I2cDevice
{
    I2cBus *bus;
    uint16_t address;
    uint32_t max_frequency_hz;
};

HwStatus i2c_transfer( const I2cDevice *device,
                       const I2cMessage *messages,
                       size_t message_count,
                       uint32_t timeout_ms );
HwStatus i2c_probe( const I2cDevice *device, uint32_t timeout_ms );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_I2C_TRANSPORT_H */
