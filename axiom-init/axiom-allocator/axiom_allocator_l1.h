/*!
 * \file axiom_allocator_l1.h
 *
 * \version     v0.8
 * \date        2016-09-23
 *
 * This file contains the AXIOM allocator level 1
 */
#ifndef AXIOM_ALLOCATOR_L1_h
#define AXIOM_ALLOCATOR_L1_h

#include "evi_alloc.h"
#include "evi_queue.h"

typedef struct axiom_allocator_l1 {
    uint64_t memory_size;
    uint64_t block_size;
    evi_alloc_t alloc_table;
    evi_queue_t app_id;
} axiom_allocator_l1_t;

void
axal_l1_init(axiom_allocator_l1_t *l1)
{
    l1->memory_size = AXIOM_ALLOCATOR_MEM_SIZE;
    l1->block_size = AXIOM_ALLOCATOR_L1_BSIZE;
    evia_init(&l1->alloc_table,
            l1->memory_size / l1->block_size);
    eviq_init(&l1->app_id, 0, AXIOM_ALLOCATOR_NUM_APP_ID);
}

static int
axal_l1_alloc_blocks(axiom_allocator_l1_t *l1, uint64_t *size,
        axiom_app_id_t app_id)
{
    int num_blocks, start;

    num_blocks = *size / l1->block_size;
    if ((num_blocks * l1->block_size) < *size)
        num_blocks++;

    *size = num_blocks * l1->block_size;

    start = evia_alloc(&l1->alloc_table, app_id, num_blocks);

    return start;
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

#endif /* AXIOM_ALLOCATOR_L1_h */
