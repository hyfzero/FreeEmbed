#ifndef TEST_LED_CONTAINER_OF_H
#define TEST_LED_CONTAINER_OF_H

#include <stddef.h>

#define container_of(ptr, type, member) \
    ( ( type * ) ( ( char * ) ( ptr ) - offsetof( type, member ) ) )

#endif /* TEST_LED_CONTAINER_OF_H */
