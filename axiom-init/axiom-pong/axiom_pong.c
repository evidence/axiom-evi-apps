/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-23
 *
 * This file tests the AXIOM INIT implementation
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_small_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "axiom_pong.h"

void axiom_pong(axiom_dev_t *dev, axiom_if_id_t first_src,
        axiom_payload_t first_payload, int verbose)
{
    axiom_err_t ret;
    axiom_ping_payload_t *payload = ((axiom_ping_payload_t *) &first_payload);

    if (payload->command != AXIOM_CMD_PING) {
        EPRINTF("receive a not AXIOM_PING message");
        return;
    }

    IPRINTF(verbose, "PING message received - src_node: %u packet_id: %u",
            first_src, payload->packet_id);

    /* send back the message */
    payload->command = AXIOM_CMD_PONG;
    ret =  axiom_send_small(dev, first_src, AXIOM_SMALL_PORT_NETUTILS,
            0, (axiom_payload_t *)payload);
    if (ret == AXIOM_RET_ERROR) {
        EPRINTF("receive error");
        return;
    }

    IPRINTF(verbose, "PONG message sent - dst_node: %u packet_id: %u",
            first_src, payload->packet_id);
}
