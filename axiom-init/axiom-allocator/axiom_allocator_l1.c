/*!
 * \file axiom_allocator_l1.c
 *
 * \version     v0.8
 * \date        2016-09-23
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the axiom-allocator messages.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "axiom_utility.h"
#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "axiom_allocator_l1.h"

#include "evi_alloc.h"
#include "evi_queue.h"

/************************* Axiom Allocator L1 CORE ****************************/
typedef struct axiom_allocator_l1 {
    uint64_t memory_start;
    uint64_t memory_size;
    uint64_t block_size;
    evi_alloc_t alloc_table;
    evi_queue_t app_id;
} axiom_allocator_l1_t;

static void
axal_l1_init(axiom_allocator_l1_t *l1)
{
    l1->memory_size = AXIOM_ALLOCATOR_MEM_SIZE;
    l1->memory_start = AXIOM_ALLOCATOR_MEM_START;
    l1->block_size = AXIOM_ALLOCATOR_L1_BSIZE;
    evia_init(&l1->alloc_table,
            l1->memory_size / l1->block_size);
    eviq_init(&l1->app_id, 0, AXIOM_ALLOCATOR_NUM_APP_ID);
}

static int
axal_l1_alloc(axiom_allocator_l1_t *l1, uint64_t *addr, uint64_t *size,
        axiom_app_id_t app_id)
{
    int num_blocks, start;

    num_blocks = *size / l1->block_size;
    if ((num_blocks * l1->block_size) < *size)
        num_blocks++;

    *size = num_blocks * l1->block_size;

    start = evia_alloc(&l1->alloc_table, app_id, num_blocks);
    if (start < 0)
        return start;

    *addr = (((uint64_t)start) * l1->block_size) + l1->memory_start;

    return 0;
}

static axiom_app_id_t
axal_l1_alloc_appid(axiom_allocator_l1_t *l1)
{
    axiom_app_id_t app_id;

    /* alloc a new application ID */
    app_id = eviq_free_pop(&l1->app_id);
    if (app_id == EVIQ_NONE) {
        return AXIOM_NULL_APP_ID;
    }

    return app_id;
}

static void
axal_l1_release(axiom_allocator_l1_t *l1, axiom_app_id_t app_id)
{
    /* release all blocks owned by app_id */
    evia_free(&l1->alloc_table, app_id);
    /* release app_id */
    eviq_free_push(&l1->app_id, app_id);
}


/*********************** Axiom Allocator L1 PROTCOL ***************************/
static axiom_allocator_l1_t l1_alloc;

void
axiom_allocator_l1_init(void)
{
    axal_l1_init(&l1_alloc);
}

int
axiom_allocator_l1_alloc(axiom_galloc_info_t *info)
{
    int ret;

    if (info->app_id == AXIOM_NULL_APP_ID)
        goto err;

    /* shared blocks */
    ret = axal_l1_alloc(&l1_alloc, &(info->shared_start),
            &(info->shared_size), info->app_id);

    if (ret < 0)
        goto err;

    /* private blocks */
    ret = axal_l1_alloc(&l1_alloc, &(info->private_start),
            &(info->private_size), info->app_id);

    if (ret < 0)
        goto err;

    info->error = AXIOM_RET_OK;
    return 1;

err:
    info->error = AXIOM_RET_ERROR;
    return 1;
}

int
axiom_allocator_l1_alloc_appid(axiom_galloc_info_t *info)
{
    info->app_id = axal_l1_alloc_appid(&l1_alloc);
    if (info->app_id == AXIOM_NULL_APP_ID)
        info->error = AXIOM_RET_ERROR;
    else
        info->error = AXIOM_RET_OK;

    return 1;
}

int
axiom_allocator_l1_release(axiom_galloc_info_t *info)
{
    axal_l1_release(&l1_alloc, info->app_id);
    return 0;
}

