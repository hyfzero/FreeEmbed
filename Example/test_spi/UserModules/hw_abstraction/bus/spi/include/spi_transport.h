#ifndef USERMODULES_SPI_TRANSPORT_H
#define USERMODULES_SPI_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "hw_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SpiBus SpiBus;
typedef struct SpiDevice SpiDevice;

/*
 * One continuous SPI segment. When tx_data is null, fill_byte is transmitted.
 * rx_data may be null when received bytes are not needed.
 */
typedef struct
{
    const uint8_t *tx_data;
    uint8_t *rx_data;
    size_t length;
    uint8_t fill_byte;
} SpiSegment;

typedef HwStatus ( *SpiTransferFn )( void *ctx,
                                    const SpiDevice *device,
                                    const SpiSegment *segments,
                                    size_t segment_count,
                                    uint32_t timeout_ms );

typedef struct
{
    SpiTransferFn transfer;
} SpiBusOps;

/*
 * A transfer callback owns the complete transaction: lock bus, configure it,
 * assert chip select, process every segment, release chip select, unlock bus.
 */
struct SpiBus
{
    const SpiBusOps *ops;
    void *ctx;
};

struct SpiDevice
{
    SpiBus *bus;
    uint32_t chip_select;
    uint32_t max_frequency_hz;
    uint8_t mode;
};

HwStatus spi_transfer( const SpiDevice *device,
                       const SpiSegment *segments,
                       size_t segment_count,
                       uint32_t timeout_ms );

#ifdef __cplusplus
}
#endif

#endif /* USERMODULES_SPI_TRANSPORT_H */
