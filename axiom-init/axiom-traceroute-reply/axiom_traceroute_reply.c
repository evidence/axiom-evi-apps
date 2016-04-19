/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-04-15
 *
 * This file implements the axiom-traceroute-reply application
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

#include "axiom_traceroute_reply.h"

axiom_err_t
axiom_next_hop(axiom_dev_t *dev, axiom_node_id_t dest_node_id,
               axiom_if_id_t *my_if) {
    axiom_err_t ret;
    int i;

    ret = axiom_get_routing(dev, dest_node_id, my_if);
    for (i = 0; i < 4; i++)
    {
        if (*my_if & (axiom_node_id_t)(1<<i))
        {
            *my_if = i;
            break;
        }
    }
    return ret;
}

void axiom_traceroute_reply(axiom_dev_t *dev, axiom_if_id_t src,
        axiom_payload_t payload, int verbose) {
    axiom_err_t err;
    axiom_err_t ret;
    axiom_if_id_t my_if;
    axiom_msg_id_t msg_err;
    axiom_msg_id_t my_node_id;
    axiom_traceroute_payload_t *recv_payload =
            ((axiom_traceroute_payload_t *) &payload);

    if (recv_payload->command != AXIOM_CMD_TRACEROUTE) {
        EPRINTF("receive a not AXIOM_CMD_TRACEROUTE message");
        return;
    }

    IPRINTF(verbose, "TRACEROUTE message received from: %u node step: %u",
            recv_payload->src_id, recv_payload->step);

    /* send reply to the node who has started the traceroute */
    recv_payload->command = AXIOM_CMD_TRACEROUTE_REPLY;
    recv_payload->step++;
    ret = axiom_send_small_init(dev, recv_payload->src_id, 0,
                                    (axiom_payload_t *)recv_payload);
    if (ret == AXIOM_RET_ERROR)
    {
        EPRINTF("send error");
        return;
    }
    IPRINTF(verbose, "TRACEROUTE_REPLY message sent with step %u",
            recv_payload->step);


    my_node_id = axiom_get_node_id(dev);
    if (my_node_id != recv_payload->dst_id)
    {
        /* get interface to reach next hop for recv_payload->dst_id node */
        err = axiom_next_hop(dev, recv_payload->dst_id, &my_if);
        if (err == AXIOM_RET_ERROR)
        {
            EPRINTF("axiom_next_hop error");
            return;
        }
        /* send small neighbour traceroute message */
        msg_err = axiom_send_small_init(dev, my_if, AXIOM_SMALL_FLAG_NEIGHBOUR,
                                        (axiom_payload_t *)&payload);

        if (msg_err == AXIOM_RET_ERROR)
        {
            EPRINTF("send small init error");
            return;
        }
    }

    return;
}