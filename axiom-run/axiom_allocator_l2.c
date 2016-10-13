/*!
 * \file axiom_allocator_l2.c
 *
 * \version     v0.8
 * \date        2016-10-13
 *
 * This file contains the AXIOM allocator level 2
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "dprintf.h"
#include "evi_alloc.h"
#include "axiom_nic_limits.h"
#include "axiom_init_api.h"
#include "axiom_global_allocator.h"
#include "axiom_allocator_l2.h"
#include "axiom_allocator_l1_l2.h"


/************************* Axiom Allocator L2 CORE ****************************/

typedef struct axiom_allocator_l2 {
    evi_alloc_t shared_alloc_table;
    uint64_t shared_start;
    uint64_t shared_size;
    uint64_t shared_block_size;
} axiom_allocator_l2_t;

static void
axal_l2_init(axiom_allocator_l2_t *l2, uint64_t shared_start,
        uint64_t shared_size)
{
    l2->shared_start = shared_start;
    l2->shared_size = shared_size;
    l2->shared_block_size = AXIOM_ALLOCATOR_L2_BSIZE;
    evia_init(&l2->shared_alloc_table,
            l2->shared_size / l2->shared_block_size);
}

static int
axal_l2_alloc(axiom_allocator_l2_t *l2, uint64_t *addr, uint64_t *size,
        axiom_node_id_t node_id)
{
    int num_blocks, start;

    num_blocks = *size / l2->shared_block_size;
    if ((num_blocks * l2->shared_block_size) < *size)
        num_blocks++;

    *size = num_blocks * l2->shared_block_size;

    start = evia_alloc(&l2->shared_alloc_table, node_id, num_blocks);
    if (start < 0)
        return start;

    *addr = (((uint64_t)start) * l2->shared_block_size) + l2->shared_start;

    return 0;
}

static void
axal_l2_free(axiom_allocator_l2_t *l2, axiom_node_id_t node_id)
{
    evia_free(&l2->shared_alloc_table, node_id);
}

static void
axal_l2_release(axiom_allocator_l2_t *l2)
{
    evia_release(&l2->shared_alloc_table);
}

/*********************** Axiom Allocator L2 PROTOCOL **************************/

typedef struct axiom_alprot_l2 {
    enum {AXL2ST_INIT = 1, AXL2ST_ALLOC, AXL2ST_SETUP, AXL2ST_RELEASE} status;
    axiom_app_id_t app_id;
    uint64_t private_start;
    uint64_t private_size;
    axiom_allocator_l2_t l2_alloc;
} axiom_alprot_l2_t;

static axiom_alprot_l2_t alprot_l2 = {
    .status = AXL2ST_RELEASE,
};

int
axiom_allocator_l2_init(axiom_app_id_t app_id)
{
    if (alprot_l2.status != AXL2ST_RELEASE) {
        EPRINTF("MASTER: allocator status [%d] unexpected", alprot_l2.status);
        return -1;
    }

    if (app_id != AXIOM_NULL_APP_ID) {
        alprot_l2.app_id = app_id;
        alprot_l2.status = AXL2ST_INIT;
    }

    return 0;
}

void
axiom_allocator_l2_release(axiom_dev_t *dev)
{
    axiom_err_t ret;

    if (alprot_l2.status == AXL2ST_SETUP) {
        ret = axal12_release(dev, alprot_l2.app_id);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("MASTER: axal12_release() error %d", ret);
        }

        axal_l2_release(&alprot_l2.l2_alloc);

        alprot_l2.status = AXL2ST_RELEASE;
    }
}

int
axiom_allocator_l2_alloc_reply(axiom_dev_t *dev, axiom_node_id_t src_node,
        size_t size, void *inmsg, axiom_galloc_info_t *info)
{
    axiom_err_t ret;

    info->error = AXIOM_RET_OK;

    if (alprot_l2.status != AXL2ST_ALLOC) {
        EPRINTF("MASTER: allocator status [%d] unexpected", alprot_l2.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    ret = axal12_alloc_parsereply(inmsg, size, &info->private_start,
            &info->private_size, &info->shared_start,
            &info->shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("MASTER: axal12_alloc_parsereply() error %d", ret);
        info->error = ret;
        goto reply;
    }

    /* setup the allocator */
    axal_l2_init(&alprot_l2.l2_alloc, info->shared_start,
            info->shared_size);

    alprot_l2.private_start = info->private_start;
    alprot_l2.private_size = info->private_size;

    alprot_l2.status = AXL2ST_SETUP;

reply:
    return 0;
}

int
axiom_allocator_l2_alloc(axiom_dev_t *dev, axiom_node_id_t src_node,
        axiom_port_t master_port, size_t size, void *buffer)
{
    axiom_galloc_info_t *info = (axiom_galloc_info_t *)buffer;
    axiom_err_t ret;

    info->error = AXIOM_RET_OK;

    if (alprot_l2.status != AXL2ST_INIT) {
        EPRINTF("MASTER: allocator status [%d] unexpected", alprot_l2.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    alprot_l2.status = AXL2ST_ALLOC;

    ret = axal12_alloc(dev, master_port, alprot_l2.app_id, info->private_size,
            info->shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("MASTER: axal12_alloc error %d", ret);
        alprot_l2.status = AXL2ST_INIT;
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    /* not send reply, we need a reply from master init */
    return 0;

reply:
    return 1;
}

int
axiom_allocator_l2_get_prblock(axiom_dev_t *dev, axiom_node_id_t src_node,
        size_t size, void *buffer)
{
    axiom_galloc_info_t *info = (axiom_galloc_info_t *)buffer;

    info->error = AXIOM_RET_OK;

    if (alprot_l2.status != AXL2ST_SETUP) {
        EPRINTF("MASTER: allocator status [%d] unexpected", alprot_l2.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    info->private_start = alprot_l2.private_start;
    info->private_size = alprot_l2.private_size;

reply:
    return 1;
}

int
axiom_allocator_l2_get_shblock(axiom_dev_t *dev, axiom_node_id_t src_node,
        size_t size, void *buffer)
{
    axiom_galloc_info_t *info = (axiom_galloc_info_t *)buffer;
    axiom_err_t ret;

    info->error = AXIOM_RET_OK;

    if (alprot_l2.status != AXL2ST_SETUP) {
        EPRINTF("MASTER: allocator status [%d] unexpected", alprot_l2.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    /* alloc new blocks TODO: handle no memory error */
    ret = axal_l2_alloc(&alprot_l2.l2_alloc, &(info->shared_start),
            &(info->shared_size), src_node);
    if (ret < 0) {
        EPRINTF("MASTER: axal_l2_alloc error %d", ret);
        info->error = AXIOM_RET_NOMEM;
        goto reply;
    }

reply:
    return 1;
}
