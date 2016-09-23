/*!
 * \file axiom_allocator.c
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

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "../axiom-init.h"
#include "evi_alloc.h"
#include "evi_queue.h"

typedef struct axiom_allocator {
    uint64_t memory_size;
    uint64_t block_size;
    evi_alloc_t alloc_table;
    evi_queue_t app_id;
} axiom_allocator_t;

static axiom_allocator_t init_allocator;

void
axiom_allocator_init()
{

    init_allocator.memory_size = AXIOM_ALLOCATOR_MEM_SIZE;
    init_allocator.block_size = AXIOM_ALLOCATOR_L1_BSIZE;
    evia_init(&init_allocator.alloc_table,
            AXIOM_ALLOCATOR_MEM_SIZE / AXIOM_ALLOCATOR_L1_BSIZE);
    eviq_init(&init_allocator.app_id, 0, AXIOM_ALLOCATOR_NUM_APP_ID);
}

static int
axiom_allocator_alloc_blocks(uint64_t *size, evia_elem_t value)
{
    int num_blocks, start;

    num_blocks = *size / AXIOM_ALLOCATOR_L1_BSIZE;
    if ((num_blocks * AXIOM_ALLOCATOR_L1_BSIZE) < *size)
        num_blocks++;

    *size = num_blocks * AXIOM_ALLOCATOR_L1_BSIZE;

    start = evia_alloc(&init_allocator.alloc_table, value, num_blocks);

    return start
}


static axiom_err_t
axiom_allocator_alloc(axiom_allocator_payload_t *alloc_payload)
{
    int start_block;

    /* shared blocks */
    start_block = axiom_allocator_alloc_blocks(&alloc_payload->shared_size,
            alloc_payload->app_id);

    if (start_block < 0)
        return AXIOM_RET_ERROR;

    alloc_payload->shared_start = start_block * AXIOM_ALLOCATOR_L1_BSIZE;

    /* private blocks */
    start_block = axiom_allocator_alloc_blocks(&alloc_payload->private_size,
            alloc_payload->app_id);

    if (start_block < 0)
        return AXIOM_RET_ERROR;

    alloc_payload->private_start = start_block * AXIOM_ALLOCATOR_L1_BSIZE;

    return AXIOM_RET_OK;
}

void
axiom_allocator(axiom_dev_t *dev, axiom_node_id_t src, size_t payload_size,
        void *payload, int verbose)
{
    axiom_err_t ret;
    axiom_allocator_payload_t *alloc_payload =
        ((axiom_allocator_payload_t *) payload);
    eviq_pnt_t app_id;

    IPRINTF(verbose, "ALLOCATOR: message received - src_node: %u", src);

    alloc_payload->error = AXIOM_RET_OK;

    switch (payload->command) {
        case AXIOM_CMD_ALLOC_APPID:
            alloc_payload->cmd = AXIOM_CMD_ALLOC_APPID_REPLY;
            /* alloc a new application ID */
            app_id = eviq_free_pop(&init_allocator.app_id);
            if (app_id == EVIQ_NONE) {
                alloc_payload->error = AXIOM_RET_ERROR;
                alloc_payload->app_id = -1;
            } else {
                alloc_payload->app_id = app_id;
            }
            break;

        case AXIOM_CMD_ALLOC:
            alloc_payload->cmd = AXIOM_CMD_ALLOC_REPLY;
            /* allocate private and shared regions */
            alloc_payload->error = axiom_allocator_alloc(alloc_payload);
            break;

        case AXIOM_CMD_ALLOC_RELEASE:
            /* release all blocks owned by app_id */
            evia_free(&init_allocator.alloc_table, alloc_payload->app_id);
            /* release app_id */
            eviq_free_push(&init_allocator.app_id, alloc_payload->app_id);
            break;

        default:
            EPRINTF("receive a not AXIOM_ALLOC message");
            alloc_payload->error = AXIOM_RET_ERROR;
            break;
    }

    /* send back the reply */
    err = axiom_send_raw(dev, src, alloc_payload->reply_port,
            AXIOM_TYPE_RAW_DATA, sizeof(*alloc_payload), alloc_payload);
    if (!AXIOM_RET_IS_OK(err)) {
        EPRINTF("ALLOCATOR: send reply error");
        return;
    }
}
