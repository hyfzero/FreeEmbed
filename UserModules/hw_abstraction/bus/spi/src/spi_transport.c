#include "spi_transport.h"

HwStatus spi_transfer( const SpiDevice *device,
                       const SpiSegment *segments,
                       size_t segment_count,
                       uint32_t timeout_ms )
{
    size_t index;

    if( ( device == 0 ) ||
        ( device->bus == 0 ) ||
        ( device->bus->ops == 0 ) ||
        ( device->bus->ops->transfer == 0 ) ||
        ( segments == 0 ) ||
        ( segment_count == 0U ) )
    {
        return HW_STATUS_INVALID_ARGUMENT;
    }

    for( index = 0U; index < segment_count; index++ )
    {
        if( segments[ index ].length == 0U )
        {
            return HW_STATUS_INVALID_ARGUMENT;
        }

        /*
         * Both buffers may be null for clock-only dummy cycles. The adapter
         * still transmits fill_byte and discards received data.
         */
    }

    return device->bus->ops->transfer( device->bus->ctx,
                                      device,
                                      segments,
                                      segment_count,
                                      timeout_ms );
}
