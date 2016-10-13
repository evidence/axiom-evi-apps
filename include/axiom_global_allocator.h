/*!
 * \file axiom_global_allocator.h
 *
 * \version     v0.8
 * \date        2016-09-26
 *
 * This file contains the AXIOM global allocator API
 *
 */
#ifndef AXIOM_GLOBAL_ALLOCATOR_h
#define AXIOM_GLOBAL_ALLOCATOR_h

#include "axiom_run_api.h"

typedef struct axiom_galloc_info {
    uint64_t shared_start;      /*!< \brief Start address of shared region */
    uint64_t shared_size;       /*!< \brief Size of shared region */
    uint64_t private_start;     /*!< \brief Start address of private region */
    uint64_t private_size;      /*!< \brief Size of private region */
    uint8_t  app_id;            /*!< \brief Application ID */
    int8_t   error;             /*!< \brief axiom_err_t error */
    uint8_t  spare[7];
} axiom_galloc_info_t;

axiom_err_t
axiom_galloc_init(axiom_node_id_t master_nodeid, axiom_node_id_t local_nodeid,
        uint64_t *private_size, uint64_t *shared_size);

axiom_err_t
axiom_galloc_get_prregion(uint64_t *start, uint64_t *size);

axiom_err_t
axiom_galloc_get_shregion(uint64_t *start, uint64_t *size);

#endif /* !AXIOM_GLOBAL_ALLOCATOR_h */
