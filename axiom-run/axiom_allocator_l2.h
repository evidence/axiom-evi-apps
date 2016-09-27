/*!
 * \file axiom_allocator_l2.h
 *
 * \version     v0.8
 * \date        2016-09-23
 *
 * This file contains the AXIOM allocator level 2
 */
#ifndef AXIOM_ALLOCATOR_L2_h
#define AXIOM_ALLOCATOR_L2_h

#include "evi_alloc.h"

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

#endif /* AXIOM_ALLOCATOR_L2_h */
