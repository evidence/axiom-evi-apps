/*!
 * \file axiom_allocator_l1.c
 *
 * \version     v0.10
 * \date        2016-09-23
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the AXIOM allocator L1 messages.
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

void
axiom_allocator_l1_init()
{
    axiom_al1_init();
}

void
axiom_allocator_l1(axiom_dev_t *dev, axiom_node_id_t src, size_t payload_size,
        void *payload, int verbose)
{
    axiom_err_t ret;
    axiom_allocator_payload_t *alloc_payload =
        ((axiom_allocator_payload_t *) payload);
    int reply = 1;

    IPRINTF(verbose, "ALLOCATOR: message received - src_node: %u", src);

    switch (alloc_payload->command) {
        case AXIOM_CMD_ALLOC_APPID:
            alloc_payload->command = AXIOM_CMD_ALLOC_APPID_REPLY;
            /* alloc a new application ID */
            reply = axiom_al1_alloc_appid(&alloc_payload->info);
            break;

        case AXIOM_CMD_ALLOC:
            alloc_payload->command = AXIOM_CMD_ALLOC_REPLY;
            /* allocate private and shared regions */
            reply = axiom_al1_alloc(&alloc_payload->info);
            break;

        case AXIOM_CMD_ALLOC_RELEASE:
            /* release the app_id and all blocks owned by app_id */
            reply = axiom_al1_release(&alloc_payload->info);
            break;

        default:
            EPRINTF("receive a not AXIOM_ALLOC message");
            alloc_payload->info.error = AXIOM_RET_ERROR;
            break;
    }

    /* send back the reply */
    if (reply) {
        ret = axiom_send_raw(dev, src, alloc_payload->reply_port,
                AXIOM_TYPE_RAW_DATA, sizeof(*alloc_payload), alloc_payload);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("ALLOCATOR: send reply error");
            return;
        }
    }
}
