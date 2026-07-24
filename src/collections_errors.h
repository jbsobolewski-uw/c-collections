//
// Common error handling contract for all c-collections modules.
//

#ifndef C_COLLECTIONS_ERRORS_H
#define C_COLLECTIONS_ERRORS_H

#include <errno.h>

/*
 * Every module aliases its own return macros to these values
 * (e.g. #define LIST_OK COLLECTIONS_OK), so all status-returning
 * functions across the library share one numbering.
 *
 * Conventions:
 *  - Constructors return a pointer; NULL means failure.
 *  - All other fallible operations return COLLECTIONS_OK or COLLECTIONS_ERR.
 *  - Query results are delivered through output parameters.
 *  - On failure errno is always set to describe the cause:
 *      EINVAL - invalid argument (NULL handle, bad output pointer, invalid enum value)
 *      ENOMEM - memory allocation failed
 *      ENOENT - element not found, or container is empty
 *      ENOSPC - resource exhausted (e.g. no identifiers left to assign)
 */
#define COLLECTIONS_OK    0
#define COLLECTIONS_ERR (-1)

#endif // C_COLLECTIONS_ERRORS_H
