//
// Created by jakub on 3/22/26.
//


#ifndef C_COLLECTIONS_ID_MANAGER_H
#define C_COLLECTIONS_ID_MANAGER_H


#include <stdint.h>
#include "../collections_errors.h"


/* Return codes */
#define ID_MANAGER_OK  COLLECTIONS_OK
#define ID_MANAGER_ERR COLLECTIONS_ERR


typedef struct IdManager id_manager_t;


// IdManager constructor.
// Returns NULL on error (sets errno = ENOMEM).
id_manager_t *idm_create(uint32_t first_id);


// IdManager destructor.
// Returns ID_MANAGER_OK, or ID_MANAGER_ERR (sets errno = EINVAL) when mgr is
// NULL.
int idm_destroy(id_manager_t *mgr);


// Assigns a new Id, possibly reusing released values.
// Returns UINT32_MAX on error (sets errno = EINVAL when mgr is NULL,
// or ENOSPC when the Id space is exhausted). Note that UINT32_MAX is also
// the last valid Id; set errno to 0 before the call to tell the two apart.
uint32_t idm_assign_id(id_manager_t *mgr);


// Returns a previously assigned Id for later reuse.
// Returns ID_MANAGER_OK on success, or ID_MANAGER_ERR (sets errno = EINVAL
// on invalid arguments, or ENOMEM on allocation failure).
int idm_release_id(id_manager_t *mgr, uint32_t id);


// Checks if there are Ids available for assignment.
// Returns 1 if an Id can be assigned, 0 otherwise
// (sets errno = EINVAL when mgr is NULL).
int idm_is_available(id_manager_t *mgr);


#endif // C_COLLECTIONS_ID_MANAGER_H
