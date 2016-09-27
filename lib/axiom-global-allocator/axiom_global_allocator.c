/*!
 * \file axiom_global_allocator.c
 *
 * \version     v0.8
 * \date        2016-09-26
 *
 * This file contains the AXIOM global allocator API implementation
 *
 */
#include <stdio.h>
#include <stdint.h>

#include "dprintf.h"
#include "axiom_nic_types.h"
#include "axiom_nic_limits.h"
#include "axiom_global_allocator.h"

#define AXIOM_ALLOCATOR_BARRIER_ID      3

axiom_err_t
axiom_galloc_init(axiom_node_id_t master_nodeid, axiom_node_id_t local_nodeid,
        uint64_t *private_size, uint64_t *shared_size)
{
    axiom_err_t ret = AXIOM_RET_OK, err;
    int verbose = 0;

    if (local_nodeid == master_nodeid) {
        axiom_galloc_info_t info;
        size_t info_size = sizeof(info);

        info.private_size = *private_size;
        info.shared_size = *shared_size;

        /* alloc private and shared regions */
        err = axrun_rpc(AXRUN_RPC_ALLOC, info_size, &info, &info_size, &info,
                verbose);
        if (err < 0 || !AXIOM_RET_IS_OK(info.error)) {
            EPRINTF("axrun_rpc error %d", err);
            ret = AXIOM_RET_NOMEM;
        }

        *private_size = info.private_size;
        *shared_size = info.shared_size;
    }

    /* barrier for all nodes */
    err = axrun_sync(AXIOM_ALLOCATOR_BARRIER_ID, verbose);
    if (err < 0) {
        EPRINTF("axrun_sync error %d", err);
        ret = AXIOM_RET_ERROR;
    }

    return ret;
}


axiom_err_t
axiom_galloc_get_prregion(uint64_t *start, uint64_t *size)
{
    axiom_galloc_info_t info;
    size_t info_size = sizeof(info);
    axiom_err_t ret;
    int verbose = 0;

    info.private_size = *size;

    ret = axrun_rpc(AXRUN_RPC_GET_PRBLOCK, info_size, &info, &info_size, &info,
            verbose);
    if (ret < 0 || !AXIOM_RET_IS_OK(info.error)) {
        EPRINTF("axrun_rpc error %d", ret);
        return AXIOM_RET_NOMEM;
    }

    *start = info.private_start;
    *size = info.private_size;

    return AXIOM_RET_OK;
}

axiom_err_t
axiom_galloc_get_shregion(uint64_t *start, uint64_t *size)
{
    axiom_galloc_info_t info;
    size_t info_size = sizeof(info);
    axiom_err_t ret;
    int verbose = 0;

    info.shared_size = *size;

    ret = axrun_rpc(AXRUN_RPC_GET_SHBLOCK, info_size, &info, &info_size, &info,
            verbose);
    if (ret < 0 || !AXIOM_RET_IS_OK(info.error)) {
        EPRINTF("axrun_rpc error %d", ret);
        return AXIOM_RET_NOMEM;
    }

    *start = info.shared_start;
    *size = info.shared_size;

    return AXIOM_RET_OK;
}
