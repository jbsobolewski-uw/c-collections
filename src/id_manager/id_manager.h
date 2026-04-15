//
// Created by jakub on 3/22/26.
//


#ifndef SIK_KAYLES_ID_MANAGER_H
#define SIK_KAYLES_ID_MANAGER_H


#include <stdint.h>


#define ID_MANAGER_OK  0    // Id manager operation successful
#define ID_MANAGER_ERR 1    // Id manager operation failed


typedef struct IdManager id_manager_t;


// IdManager constructor
id_manager_t *idm_create(uint32_t first_id);


// IdManager destructor
void idm_destroy(id_manager_t *mgr);


// Assigns a new Id, possibly reusing returned values
uint32_t idm_assign_id(id_manager_t *mgr);


// Return a previously assigned Id for later reuse
int idm_release_id(id_manager_t *mgr, uint32_t id);


// Checks if there are Id available for assignment
int idm_is_available(id_manager_t *mgr);


#endif //SIK_KAYLES_ID_MANAGER_H
