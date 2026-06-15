#include "i2c_transport.h"

HwStatus i2c_transfer( const I2cDevice *device,
                       const I2cMessage *messages,
                       size_t message_count,
                       uint32_t timeout_ms )
{
    size_t message_index;
    size_t segment_index;

    if( ( device == 0 ) ||
        ( device->bus == 0 ) ||
        ( device->bus->ops == 0 ) ||
        ( device->bus->ops->transfer == 0 ) ||
        ( messages == 0 ) ||
        ( message_count == 0U ) )
    {
        return HW_STATUS_INVALID_ARGUMENT;
    }

    for( message_index = 0U; message_index < message_count; message_index++ )
    {
        const I2cMessage *message = &messages[ message_index ];

        if( ( message->segments == 0 ) || ( message->segment_count == 0U ) )
        {
            return HW_STATUS_INVALID_ARGUMENT;
        }

        if( ( message->flags & ~I2C_MESSAGE_READ ) != 0U )
        {
            return HW_STATUS_INVALID_ARGUMENT;
        }

        for( segment_index = 0U;
             segment_index < message->segment_count;
             segment_index++ )
        {
            const I2cSegment *segment = &message->segments[ segment_index ];

            if( segment->length == 0U )
            {
                return HW_STATUS_INVALID_ARGUMENT;
            }

            if( ( message->flags & I2C_MESSAGE_READ ) != 0U )
            {
                if( segment->rx_data == 0 )
                {
                    return HW_STATUS_INVALID_ARGUMENT;
                }
            }
            else if( segment->tx_data == 0 )
            {
                return HW_STATUS_INVALID_ARGUMENT;
            }
        }
    }

    return device->bus->ops->transfer( device->bus->ctx,
                                      device,
                                      messages,
                                      message_count,
                                      timeout_ms );
}

HwStatus i2c_probe( const I2cDevice *device, uint32_t timeout_ms )
{
    if( ( device == 0 ) ||
        ( device->bus == 0 ) ||
        ( device->bus->ops == 0 ) ||
        ( device->bus->ops->probe == 0 ) )
    {
        return HW_STATUS_INVALID_ARGUMENT;
    }

    return device->bus->ops->probe( device->bus->ctx, device, timeout_ms );
}
