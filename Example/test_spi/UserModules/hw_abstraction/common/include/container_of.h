#ifndef USERMODULES_CONTAINER_OF_H
#define USERMODULES_CONTAINER_OF_H

#include <stddef.h>

/*
 * Get the outer structure pointer from one of its member pointers.
 *
 * Parameters:
 *   ptr    Pointer to the structure member.
 *   type   Type of the outer structure.
 *   member Field name of the member inside type.
 */
#define container_of(ptr, type, member) \
    ( ( type * ) ( ( char * ) ( ptr ) - offsetof( type, member ) ) )

#endif /* USERMODULES_CONTAINER_OF_H */
