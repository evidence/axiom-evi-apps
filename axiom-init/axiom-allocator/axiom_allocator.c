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

#include "axiom_utility.h"
#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "../axiom-init.h"
#include "axiom_allocator_l1.h"

static axiom_allocator_l1_t l1_alloc;

void
axiom_allocator_init()
{
    axal_l1_init(&l1_alloc);
}

static axiom_err_t
axiom_allocator_alloc(axiom_allocator_payload_t *alloc_payload)
{
    int start_block;

    /* shared blocks */
    start_block = axal_l1_alloc_blocks(&l1_alloc, &alloc_payload->shared_size,
            alloc_payload->app_id);

    if (start_block < 0)
        return AXIOM_RET_ERROR;

    alloc_payload->shared_start = start_block * AXIOM_ALLOCATOR_L1_BSIZE;

    /* private blocks */
    start_block = axal_l1_alloc_blocks(&l1_alloc, &alloc_payload->private_size,
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

    IPRINTF(verbose, "ALLOCATOR: message received - src_node: %u", src);

    alloc_payload->error = AXIOM_RET_OK;

    switch (alloc_payload->command) {
        case AXIOM_CMD_ALLOC_APPID:
            alloc_payload->command = AXIOM_CMD_ALLOC_APPID_REPLY;
            /* alloc a new application ID */
            alloc_payload->app_id = axal_l1_alloc_appid(&l1_alloc);
            if (alloc_payload->app_id == AXIOM_NULL_APP_ID) {
                alloc_payload->error = AXIOM_RET_ERROR;
            }
            break;

        case AXIOM_CMD_ALLOC:
            alloc_payload->command = AXIOM_CMD_ALLOC_REPLY;
            /* allocate private and shared regions */
            alloc_payload->error = axiom_allocator_alloc(alloc_payload);
            break;

        case AXIOM_CMD_ALLOC_RELEASE:
            /* release the app_id and all blocks owned by app_id */
            axal_l1_release(&l1_alloc, alloc_payload->app_id);
            break;

        default:
            EPRINTF("receive a not AXIOM_ALLOC message");
            alloc_payload->error = AXIOM_RET_ERROR;
            break;
    }

    /* send back the reply */
    ret = axiom_send_raw(dev, src, alloc_payload->reply_port,
            AXIOM_TYPE_RAW_DATA, sizeof(*alloc_payload), alloc_payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("ALLOCATOR: send reply error");
        return;
    }
}
