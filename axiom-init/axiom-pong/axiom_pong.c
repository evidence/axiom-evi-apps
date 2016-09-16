/*!
 * \file axiom_pong.c
 *
 * \version     v0.8
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the axiom-ping/pong messages.
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

void
axiom_pong(axiom_dev_t *dev, axiom_if_id_t first_src,
        axiom_init_payload_t *first_payload, int verbose)
{
    axiom_err_t ret;
    axiom_ping_payload_t *payload = ((axiom_ping_payload_t *) first_payload);

    if (payload->command != AXIOM_CMD_PING) {
        EPRINTF("receive a not AXIOM_PING message");
        return;
    }

    IPRINTF(verbose, "PING message received - src_node: %u unique_id: %u "
            "seq_num: %u",
            first_src, payload->unique_id, payload->seq_num);

    /* send back the message */
    payload->command = AXIOM_CMD_PONG;
    ret =  axiom_send_raw(dev, first_src, AXIOM_RAW_PORT_NETUTILS,
            AXIOM_TYPE_RAW_DATA, sizeof(*payload), payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("sending error");
        return;
    }

    IPRINTF(verbose, "PONG message sent - dst_node: %u unique_id: %u "
            "seq_num: %u",
            first_src, payload->unique_id, payload->seq_num);
}
