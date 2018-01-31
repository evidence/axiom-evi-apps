/*!
 * \file axiom_discovery_protocol.c
 *
 * \version     v0.15
 * \date        2016-05-03
 *
 * This file contains the implementation of the axiom discovery phase.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#include "axiom_discovery_protocol.h"
#include "axiom_nic_discovery.h"

extern int verbose;

/* Initializes the gloabl Topology matrix of a node */
static void
axiom_topology_init(axiom_node_id_t topology[][AXIOM_INTERFACES_NUM])
{
    int i,j;

    for (i = 0; i < AXIOM_NODES_NUM; i++) {
        for (j = 0; j < AXIOM_INTERFACES_NUM; j++) {
            topology[i][j] = AXIOM_NULL_NODE;
        }
    }
}

/* Codify the mask to set into the routing table */
inline static uint8_t
axiom_codify_routing_mask(axiom_if_id_t if_id)
{
    return (uint8_t)(1 << if_id);
}

/* reset local routing table */
static void
axiom_routing_init(axiom_dev_t *dev)
{
    int node_id;

    for (node_id = 0; node_id < AXIOM_NODES_NUM; node_id++) {
        axiom_set_routing(dev, node_id, 0);
    }

}

/* Master and Slave common part of the discover algorithm */
static int
discover_phase(axiom_dev_t *dev, axiom_node_id_t *next_id,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM])
{
    axiom_if_id_t num_interface = 0;
    uint8_t if_features = 0;
    axiom_discovery_cmd_t msg_cmd = 0;
    axiom_node_id_t src_node_id, dst_node_id;
    axiom_if_id_t src_interface;
    axiom_if_id_t data_src_if, data_dst_if;
    axiom_node_id_t start_node_index, node_index;
    int i, a, b;
    uint8_t b_mask;
    axiom_err_t ret;

    /* The node reads its node id (1) */
    axiom_node_id_t node_id = axiom_get_node_id(dev);

    /* Immediately update loopback interface */
    b_mask = axiom_codify_routing_mask(AXIOM_IF_LOOPBACK);
    axiom_set_routing(dev, node_id, b_mask);

    IPRINTF(verbose, "node_id = %d", node_id);

    /* The node reads the info about the interfaces */
    axiom_get_if_number(dev, &num_interface);

    IPRINTF(verbose, "Number of interface = %d", num_interface);

    /* For each interface */
    for (i = 0; i < num_interface; i++) {
        /* get interface feature*/
        axiom_get_if_info(dev, i, &if_features);
        /* the interface 'i' is physically connected to another board*/
        if ((if_features & AXIOM_IF_CONNECTED) == 0) {
            continue;
        }
        /* The node has to poll the interface only if the topology table
         * indicates that on the interface, the neighour node id is unknown.
         * Infact if the node on that interface has already an ID and node
         * sends it a message, node will be blocked waiting a response that
         * the neighbour (that has alreay ended its protocol) will never
         * send it.
         */
        if (topology[node_id][i] != AXIOM_NULL_NODE) {
            continue;
        }
        IPRINTF(verbose, "Node:%d, Send to interface number = %d "
                "the AXIOM_DSCV_CMD_REQ_ID message", node_id, i);

        /* Say over interface 'i': 'I am node node_id give me your node id' */
        ret = axiom_send_raw_discovery(dev, i, AXIOM_DSCV_CMD_REQ_ID,
                node_id, 0, 0, 0);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("Node:%d, Error sending to interface number = %d the "
                    "AXIOM_DSCV_CMD_REQ_ID message", node_id, i);
            break;
        }
        IPRINTF(verbose, "Node:%d, Wait for receiving response on interface number = %d",
                node_id, i);

        /* Wait for the neighbour response */
        ret = axiom_recv_raw_discovery(dev, &src_interface, &msg_cmd,
                &src_node_id, &dst_node_id, &data_src_if, &data_dst_if);

        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("Node:%d, Error receiving response on interface number = %d",
                    node_id, i);
            break;
        }

        /* message received form the 'i' interface and it is the exepected message */
        if (!((msg_cmd == AXIOM_DSCV_CMD_RSP_NOID) ||
                    (msg_cmd == AXIOM_DSCV_CMD_RSP_ID))) {
            EPRINTF("Node:%d, command [%d] not exepected", node_id, msg_cmd);
            break;
        }

        /* if the receiving side says 'I am node X, I'm on interface Y' */
        if (msg_cmd == AXIOM_DSCV_CMD_RSP_ID) {
            IPRINTF(verbose, "Node:%d, Received from interface %d the message "
                    "AXIOM_DSCV_CMD_RSP_ID", node_id, i);

            /* Update the topology data structure */
            topology[node_id][src_interface] = src_node_id;
            topology[src_node_id][data_src_if] = node_id;

            IPRINTF(verbose, "Node:%d, Update local topology structure:", dst_node_id);
            IPRINTF(verbose, "Node:%d If:%d Connected to Node:%d", node_id,
                    src_interface, src_node_id);
            IPRINTF(verbose, "Node:%d If:%d Connected to Node:%d", src_node_id,
                    src_node_id, node_id);
            continue;
        }

        /* otherwise the receiving side says 'I do not know, I'm on interface Y' */
        IPRINTF(verbose, "Node:%d, Received from interface %d the message "
                "AXIOM_DSCV_CMD_RSP_NOID", node_id, i);

        (*next_id)++;

        IPRINTF(verbose, "Node:%d, Send to interface number = %d the AXIOM_DSCV_CMD_SETID "
                "message, id_node=%d", node_id, i, *next_id);

        /* Say over interface 'i': you are node 'nextid' */
        ret = axiom_send_raw_discovery(dev, i, AXIOM_DSCV_CMD_SETID,
                node_id, *next_id, 0, 0);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("Node:%d, Error sending to interface number = %d the "
                    "AXIOM_DSCV_CMD_SETID message, id_node=%d",
                    node_id, i, *next_id);
            break;
        }

        /* Immediately update local routing table: next_id node is connceted
         * to local 'i' interface */
        //axiom_get_routing(dev, *next_id, &b_mask);
        b_mask = axiom_codify_routing_mask(i);
        axiom_set_routing(dev, *next_id, b_mask);
        IPRINTF(verbose, "Node:%d, Set local routing table node: %d "
                "if_mask: 0x%x", node_id, *next_id, b_mask);

        /* Update the topology data structure:  local 'i' interface is connected
         * the new next_id node and next_id node is connected to me */
        topology[node_id][i] = *next_id;
        topology[*next_id][data_src_if] = node_id;

        IPRINTF(verbose, "Node:%d, Update local topology structure:", node_id);
        IPRINTF(verbose, "Node:%d If:%d Connected to Node:%d", node_id, i, *next_id);
        IPRINTF(verbose, "Node:%d If:%d Connected to Node:%d", *next_id, data_src_if,
                node_id);

        IPRINTF(verbose, "Node:%d, Send to interface number = %d the AXIOM_DSCV_CMD_START "
                "message", node_id, i);
        /* Say over interface 'i': start discovery protocol, nextid */
        ret = axiom_send_raw_discovery(dev, i, AXIOM_DSCV_CMD_START,
                node_id, *next_id, 0, 0);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("Node:%d, Error sending to interface number = %d the "
                    "AXIOM_DSCV_CMD_START message", node_id, i);
            break;
        }
        start_node_index = *next_id;

        msg_cmd = 0;
        /* wait for the messages with the next_id topology data structure */
        while (msg_cmd != AXIOM_DSCV_CMD_END_TOPOLOGY) {
            IPRINTF(verbose, "Node:%d, Wait for AXIOM_DSCV_CMD_END_TOPOLOGY/AXIOM_DSCV_CMD_TOPOLOGY "
                    "messages", node_id);

            /* Receive back the  next_id neighbour node topology data
             * structure and the new updated nextid or an ID request */
            ret = axiom_recv_raw_discovery(dev, &src_interface, &msg_cmd,
                    &src_node_id, &dst_node_id, &data_src_if, &data_dst_if);
            if (!AXIOM_RET_IS_OK(ret)) {
                EPRINTF("Node:%d, Error in receiving while waiting for "
                        "AXIOM_DSCV_CMD_END_TOPOLOGY/AXIOM_DSCV_CMD_TOPOLOGY "
                        "messages", node_id);
                break;
            }
            /* request for local id from a node which is executing its
             * discovery algorithm */
            if (msg_cmd == AXIOM_DSCV_CMD_REQ_ID) {
                /* Immediately update local routing table: src_node_id node is
                 * connceted to local 'src_interface' interface */

                /* Reply 'I am node 'node_id', I'm on interface 'src_interface' */
                ret = axiom_send_raw_discovery(dev, src_interface,
                        AXIOM_DSCV_CMD_RSP_ID,
                        node_id, src_node_id, src_interface, 0);
                IPRINTF(verbose, "Node:%d, Send AXIOM_DSCV_CMD_RSP_ID on if %d",
                        node_id, src_interface);
                if (!AXIOM_RET_IS_OK(ret)) {
                    EPRINTF("Node:%d, Error sending AXIOM_DSCV_CMD_RSP_ID messages",
                            node_id);
                    break;
                }
            /* request of memorizing topology info */
            } else if (msg_cmd == AXIOM_DSCV_CMD_TOPOLOGY) {
                /* Update the topology data structure */
                topology[src_node_id][data_src_if] = dst_node_id;
                topology[dst_node_id][data_dst_if] = src_node_id;

                IPRINTF(verbose, "Node:%d, RECEIVE_TOPOLOGY", node_id);
                IPRINTF(verbose, "%d %d Connected To %d", src_node_id, data_src_if,
                        dst_node_id);
                IPRINTF(verbose, "%d %d Connected To %d", dst_node_id, data_dst_if,
                        src_node_id);

            } else if (msg_cmd == AXIOM_DSCV_CMD_END_TOPOLOGY) {
                IPRINTF(verbose, "Node:%d, AXIOM_DSCV_CMD_END_TOPOLOGY messages received",
                        node_id);

                /* get the updated nextid */
                *next_id = src_node_id;

                /* Update routing table of all nodes discovered by the node to
                 * which I request to execute the discovery algorithm */
                for (node_index = start_node_index; node_index <= (*next_id);
                        node_index++) {
                    //axiom_get_routing(dev, node_index, &b_mask);
                    b_mask = axiom_codify_routing_mask(i);
                    axiom_set_routing(dev, node_index, b_mask);
                }
                NDPRINTF("Node:%d TOPOLOGY\n", node_id);
                for (a = 0; a < AXIOM_NODES_NUM; a++) {
                    for (b = 0; b < AXIOM_INTERFACES_NUM; b++)
                        NDPRINTF("%d ", topology[a][b]);
                }
            }
        }
    }

    return AXIOM_RET_OK;
}

/* Master node Discovery Algorithm code */
int
axiom_master_node_discovery(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_node_id_t master_id, axiom_node_id_t *last_node)
{
    axiom_node_id_t next_id = master_id;
    int ret;

    /* init local topolgy structure and routing table */
    axiom_topology_init(topology);
    axiom_routing_init(dev);

    /* set master node id */
    axiom_set_node_id(dev, master_id);

    ret = discover_phase(dev, &next_id, topology);
    *last_node = (next_id + 1);

    return ret;
}

/* Slave nodes Discovery Algorithm code */
int
axiom_slave_node_discovery (axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_node_id_t *node_id, axiom_if_id_t first_interface,
        axiom_discovery_payload_t *first_msg)
{
    axiom_node_id_t next_id;
    axiom_discovery_cmd_t msg_cmd ;
    axiom_node_id_t src_node_id, dst_node_id = 0;
    axiom_if_id_t src_interface, data_src_if, data_dst_if;
    int i, j, w;
    uint8_t b_mask;
    axiom_err_t ret;

    /* called when received the first AXIOM_DSCV_CMD_REQ_ID type message */
    src_interface = first_interface;
    src_node_id = first_msg->src_node;
    msg_cmd = first_msg->command;

    if (msg_cmd != AXIOM_DSCV_CMD_REQ_ID) {
        EPRINTF("Slave: Expected AXIOM_DSCV_CMD_REQ_ID message");
        return AXIOM_RET_ERROR;
    }

    /* init local topolgy structure and routing table */
    axiom_topology_init(topology);
    axiom_routing_init(dev);

    /* reset id when discovery phase starts */
    *node_id = 0;
    axiom_set_node_id(dev, *node_id);
    *node_id = axiom_get_node_id(dev);

    IPRINTF(verbose, "Slave: node_id = %d neighbour_node_id = %d",
            *node_id, src_node_id);

    /* Immediately update local routing table: src_node_id node is connceted to
     * local 'src_interface' interface */
    //axiom_get_routing(dev, src_node_id, &b_mask);
    b_mask = axiom_codify_routing_mask(src_interface);
    axiom_set_routing(dev, src_node_id, b_mask);

    /* If I have a node _id */
    if (*node_id != 0) {
        IPRINTF(verbose, "Slave: send to interface number = %d the "
                "AXIOM_DSCV_CMD_RSP_ID message",
                src_interface);
        /* Reply 'I am node 'node_id', I'm on interface 'src_interface' */
        ret = axiom_send_raw_discovery(dev, src_interface, AXIOM_DSCV_CMD_RSP_ID,
                *node_id, src_node_id, src_interface, 0);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("Slave id:%d Error sending AXIOM_DSCV_CMD_RSP_ID message",
                    *node_id);
        }
        return ret;
    }

    IPRINTF(verbose, "Slave: send to interface number = %d the "
            "AXIOM_DSCV_CMD_RSP_NOID message",
            src_interface);

    /* Reply 'I do not have an id, I'm on interface src_interface' */
    ret = axiom_send_raw_discovery(dev, src_interface, AXIOM_DSCV_CMD_RSP_NOID,
            *node_id, src_node_id, src_interface, 0);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("Slave: error sending to interface number = %d the "
                "AXIOM_DSCV_CMD_RSP_NOID message", src_interface);
        return ret;
    }
    IPRINTF(verbose, "Slave: Wait for AXIOM_DSCV_CMD_SETID message");

    /* Wait for the neighbour AXIOM_DSCV_CMD_SETID type message */
    ret = AXIOM_RET_OK;
    while ((msg_cmd != AXIOM_DSCV_CMD_SETID) && AXIOM_RET_IS_OK(ret)) {
        ret = axiom_recv_raw_discovery(dev, &src_interface, &msg_cmd,
                &src_node_id, &dst_node_id, &data_src_if, &data_dst_if);
    }
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("Slave: Error receving AXIOM_DSCV_CMD_SETID message, local id =%d",
                dst_node_id);
        return ret;
    }
    IPRINTF(verbose, "Slave: Received AXIOM_DSCV_CMD_SETID message, local id =%d",
            dst_node_id);

    /* set local id*/
    axiom_set_node_id(dev, dst_node_id);
    *node_id = dst_node_id;

    /* Wait for the neighbour AXIOM_DSCV_CMD_START type message */
    ret = AXIOM_RET_OK;
    while ((msg_cmd != AXIOM_DSCV_CMD_START) && AXIOM_RET_IS_OK(ret)) {
        IPRINTF(verbose, "Slave: Wait for AXIOM_DSCV_CMD_START message - msg_cmd: %x",
                msg_cmd);

        ret = axiom_recv_raw_discovery(dev, &src_interface, &msg_cmd,
                &src_node_id, &dst_node_id, &data_src_if, &data_dst_if);
    }
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("Slave id:%d Error receiving AXIOM_DSCV_CMD_START message",
                *node_id);
        return ret;
    }
    next_id = dst_node_id; /* that is local id, previously yet recevied into data field! */

    IPRINTF(verbose, "Slave id:%d Received AXIOM_DSCV_CMD_START message, I start "
            "discovery phase ", *node_id);

    /* Execute the dicovery algorithm*/
    discover_phase(dev, &next_id, topology);

    IPRINTF(verbose, "Node:%d Ended discovery phase, send local topology table to _node:%d "
            "on local interface:%d", *node_id, src_node_id, src_interface);

    /* Send topology list to the node which sent me the start message */
    for (i = 0; i < AXIOM_NODES_NUM; i ++) {
        for (j = 0; j < AXIOM_INTERFACES_NUM; j++) {
            if (topology[i][j] == AXIOM_NULL_NODE) {
                continue;
            }
            /* find the remote node interface */
            for(w = 0; w < AXIOM_INTERFACES_NUM; w++) {
                if (topology[topology[i][j]][w] == i)
                    break;
            }
            /* there is a link to return */
            ret = axiom_send_raw_discovery(dev, src_interface,
                    AXIOM_DSCV_CMD_TOPOLOGY, i, topology[i][j], j, w);
            if (!AXIOM_RET_IS_OK(ret)) {
                EPRINTF("Slave id:%d Error receiving AXIOM_DSCV_CMD_START message",
                        *node_id);
                return ret;
            }
            DPRINTF("%d %d %d %d",i, j, topology[i][j], w);
        }
    }

    IPRINTF(verbose, "Node:%d Send AXIOM_DSCV_CMD_END_TOPOLOGY message to Node:%d on local "
            "interface:%d", *node_id, src_node_id, src_interface);

    /* Says: <<Finished sending the topology structure,
     * I send back actual next_id>> */
    ret = axiom_send_raw_discovery(dev, src_interface,
            AXIOM_DSCV_CMD_END_TOPOLOGY, next_id, 0, 0, 0);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("Slave id:%d Error sending AXIOM_DSCV_CMD_END_TOPOLOGY message",
                *node_id);
        return ret;
    }

    return ret;
}

