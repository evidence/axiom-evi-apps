/*!
 * \file axiom_traceroute_reply.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the axiom-traceroute messages.
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

#include "../axiom-init.h"

void
axiom_traceroute_reply(axiom_dev_t *dev, axiom_if_id_t src,
        axiom_payload_t payload, int verbose) {
    axiom_err_t ret;
    axiom_if_id_t if_id;
    axiom_msg_id_t msg_err;
    axiom_node_id_t node_id;
    axiom_payload_t send_payload;
    axiom_traceroute_payload_t *recv_payload =
            ((axiom_traceroute_payload_t *) &payload);

    if (recv_payload->command != AXIOM_CMD_TRACEROUTE) {
        EPRINTF("receive a not AXIOM_CMD_TRACEROUTE message");
        return;
    }

    IPRINTF(verbose, "TRACEROUTE message received from: %u node step: %u",
            recv_payload->src_id, recv_payload->step);

    recv_payload->step++;
    send_payload = payload;

    /* send reply to the node who has started the traceroute */
    recv_payload->command = AXIOM_CMD_TRACEROUTE_REPLY;
    ret = axiom_send_small(dev, recv_payload->src_id, AXIOM_SMALL_PORT_NETUTILS,
            AXIOM_SMALL_TYPE_DATA, (axiom_payload_t *)recv_payload);
    if (ret == AXIOM_RET_ERROR) {
        EPRINTF("send error");
        return;
    }
    IPRINTF(verbose, "TRACEROUTE_REPLY message sent with step %u",
            recv_payload->step);


    node_id = axiom_get_node_id(dev);
    if (node_id != recv_payload->dst_id) {
        /* get interface to reach next hop for recv_payload->dst_id node */
        ret = axiom_next_hop(dev, recv_payload->dst_id, &if_id);
        if (ret == AXIOM_RET_ERROR) {
            EPRINTF("axiom_next_hop error");
            return;
        }
        /* send small neighbour traceroute message */
        msg_err = axiom_send_small_init(dev, if_id, AXIOM_SMALL_TYPE_NEIGHBOUR,
                                        (axiom_payload_t *)&send_payload);

        if (msg_err == AXIOM_RET_ERROR) {
            EPRINTF("send small init error");
            return;
        }
    }

    return;
}
