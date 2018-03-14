/*!
 * \file axiom_net_switch.c
 *
 * \version     v1.2
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "axiom_nic_packets.h"
#include "axiom_nic_routing.h"
#include "axiom_simulator.h"
#include "axiom_net.h"

#include "axiom_switch_packets.h"

typedef struct axiom_net {
    int switch_fd;
    int connected_if[AXIOM_INTERFACES_NUM];
} axiom_net_t;

#define AXSW_PORT_START         33300   /* first port to listen */

/*
 * @brief This function allocates a socket for each node in order to
 *        communicate with the remote switch
 * @param nodeA AXIOM node
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns -1 in case of error
 *                 0 otherwise
 */
static int
allocate_socket_node(uint8_t nodeA, axiom_sim_node_args_t *nodes,
        axiom_sim_topology_t *tpl)
{
    uint8_t found_flag = 0;
    int i, socket_d, ret;
    struct sockaddr_in addr;

    for (i = 0; i < tpl->num_interfaces; i++) {
        if (tpl->topology[nodeA][i] != AXIOM_NULL_NODE) {
            nodes[nodeA].net->connected_if[i] = AXIOM_IF_CONNECTED;
            found_flag = 1;
        }
        else {
            nodes[nodeA].net->connected_if[i] = AXIOM_NULL_INTERFACE;
        }
    }
    if (found_flag == 0) {
        DPRINTF(" node %d disconnected from the network", nodeA);
        return -1;
    }

    /* Create a socket for nodeA */
    socket_d = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_d  < 0) {
        perror("opening stream socket ");
        return -1;
    }
    nodes[nodeA].net->switch_fd = socket_d;
    DPRINTF("Node %d - Socket %d", nodeA, socket_d);

    /* Connect to the Axiom-Switch ports */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = htons(AXSW_PORT_START + nodeA);

    ret = connect(socket_d, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("connect failed");
        return -1;
    }
    DPRINTF("Node %d connected", nodeA);

    return 0;
}

/*
 * @brief This function sets up the nodes resources for discovery
 *        algorithm execution
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns -1 in case of error
 *                 0 otherwise
 */
int
axiom_net_setup(axiom_sim_node_args_t *nodes, axiom_sim_topology_t *tpl)
{
    int i, err;

    for (i = 0; i < tpl->num_nodes; i++) {
        nodes[i].net = malloc(sizeof (axiom_net_t));
        if (!nodes[i].net) {
            axiom_net_free(nodes, tpl);
            return -ENOMEM;
        }
    }

    for (i = 0; i < tpl->num_nodes; i++) {
        err = allocate_socket_node(i, nodes, tpl);
        if (err) {
            return err;
        }
    }

    return 0;
}

/*
 * @brief This function frees the memory allocated during the start
 *        up nodes phase
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns none
 */
void
axiom_net_free(axiom_sim_node_args_t *nodes, axiom_sim_topology_t *tpl)
{
    int i;

    for (i = 0; i < tpl->num_nodes; i++) {
        if (nodes[i].net) {
            free(nodes[i].net);
            nodes[i].net = NULL;
        }
    }
}

/*
 * @brief This function returns the connected status of the input
 *        node interface
 * @param dev pointer to the node information
 * @param if_number node interface identifier
 * return Returns 1 interface connected
 *                0 otherwise
 */
int
axiom_net_connect_status(axiom_dev_t *dev, axiom_if_id_t if_number)
{
    return (((axiom_sim_node_args_t*)dev)->net->connected_if[if_number]
            != AXIOM_NULL_INTERFACE);
}

/*
 * @brief This function sends a raw message to a neighbour on a specific
 *        interface.
 * @param dev The axiom devive private data pointer
 * @param src_interface Sender interface identification
 * @param type type of the raw message
 * @param data Data to send
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_send_raw_neighbour(axiom_dev_t *dev, axiom_if_id_t src_interface,
        axiom_port_t port, axiom_type_t type, axiom_raw_payload_t *payload)
{
    axiom_raw_msg_t message;
    axiom_raw_eth_t raw_eth;
    uint32_t axiom_msg_length = sizeof(axiom_raw_eth_t);
    int ret;

    /* Header */
    message.header.tx.port_type.field.port = port;
    message.header.tx.port_type.field.type = type;
    message.header.tx.dst = src_interface;
    /* Payload */
    message.payload = *payload;


    /* ethernet packet management */
    axiom_msg_length = htonl(axiom_msg_length);
    ret = send(((axiom_sim_node_args_t*)dev)->net->switch_fd, &axiom_msg_length,
            sizeof(axiom_msg_length), 0);
    if (ret != sizeof(axiom_msg_length)) {
        EPRINTF("axiom_msg_length send error - return: %d", ret);
        return AXIOM_RET_ERROR;
    }
    raw_eth.eth_hdr.type = htons(AXIOM_ETH_TYPE_RAW);
    memcpy(&raw_eth.raw_msg, &message, sizeof(message));

    /* send the ethernet packet */
    axiom_msg_length = ntohl(axiom_msg_length);
    ret = send( ((axiom_sim_node_args_t*)dev)->net->switch_fd, &raw_eth,
            axiom_msg_length, 0);

    if (ret <= 0) {
        return AXIOM_RET_ERROR;
    }
    return AXIOM_RET_OK;
}

/*
 * @brief This function sends a raw message to a node
 * @param dev The axiom devive private data pointer
 * @param src_node_id Sender node identification
 * @param dest_node_id Recipient node identification
 * @param type type of the raw message
 * @param data Data to send
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_send_raw(axiom_dev_t *dev, axiom_if_id_t dest_node_id,
        axiom_port_t port, axiom_type_t type, axiom_raw_payload_t *payload)
{
    axiom_raw_msg_t message;
    axiom_raw_eth_t raw_eth;
    uint32_t axiom_msg_length = sizeof(axiom_raw_eth_t);
    int ret;

    if (port != AXIOM_RAW_PORT_INIT)
    {
        return AXIOM_RET_ERROR;
    }

    /* Header message */
    message.header.tx.port_type.field.port = port;
    message.header.tx.port_type.field.type = type;
    message.header.tx.dst = dest_node_id;
    /* Payload */
    message.payload= *payload;

    /* Ethernet packet management */
    axiom_msg_length = htonl(axiom_msg_length);
    ret = send(((axiom_sim_node_args_t*)dev)->net->switch_fd, &axiom_msg_length,
            sizeof(axiom_msg_length), 0);
    if (ret != sizeof(axiom_msg_length)) {
        EPRINTF("axiom_msg_length send error - return: %d", ret);
        return AXIOM_RET_ERROR;
    }
    raw_eth.eth_hdr.type = htons(AXIOM_ETH_TYPE_RAW);
    memcpy(&raw_eth.raw_msg, &message, sizeof(message));

    /* Send the ethernet packet */
    axiom_msg_length = ntohl(axiom_msg_length);
    ret = send( ((axiom_sim_node_args_t*)dev)->net->switch_fd, &raw_eth,
            axiom_msg_length, 0);
    if (ret <= 0) {
        return AXIOM_RET_ERROR;
    }
    DPRINTF("routing: send on socket = %d to node %d: (%d,%d)",
            ((axiom_sim_node_args_t*)dev)->net->switch_fd, message.header.tx.dst,
            (uint8_t)((message.payload & 0x00FF0000) >> 16),
            (uint8_t)((message.payload & 0x0000FF00) >> 8));

    return AXIOM_RET_OK;
}

/*
 * @brief This function receives raw neighbour data.
 * @param dev The axiom devive private data pointer
 * @param src_interface The source node id that sent the raw data or local
 *               interface that received the raw data
 * @param port port of the raw message
 * @param type type of the raw message
 * @param payload data received
 * @return Returns a unique positive message id on success, -1 otherwise.
 * XXX: the return type is unsigned!
 */
axiom_msg_id_t
axiom_net_recv_raw_neighbour(axiom_dev_t *dev, axiom_node_id_t *src_interface,
        axiom_port_t *port, axiom_type_t *type, axiom_raw_payload_t *payload)
{
    axiom_raw_eth_t raw_eth;
    uint32_t axiom_msg_length;
    int ret, retry = 1;

    while (retry) {
        /* receive the length of the ethernet packet */
        ret = recv(((axiom_sim_node_args_t*)dev)->net->switch_fd,
                &axiom_msg_length, sizeof(axiom_msg_length), MSG_WAITALL);
        if (ret < 0) {
            return AXIOM_RET_ERROR;
        }

        axiom_msg_length = ntohl(axiom_msg_length);
        if (axiom_msg_length > sizeof(raw_eth)) {
            EPRINTF("too long message - len: %d", axiom_msg_length);
            return AXIOM_RET_ERROR;
        }

        /* receive ethernet packet */
        ret = recv(((axiom_sim_node_args_t*)dev)->net->switch_fd, &raw_eth,
                axiom_msg_length, MSG_WAITALL);
        if (ret != axiom_msg_length) {
            EPRINTF("unexpected length - expected: %d received: %d",
                    axiom_msg_length, ret);
            return AXIOM_RET_ERROR;
        }

        NDPRINTF("Receive from socket number = %d",
                ((axiom_sim_node_args_t*)dev)->net->switch_fd);

        if (raw_eth.eth_hdr.type != htons(AXIOM_ETH_TYPE_RAW)) {
            retry = 1;
            DPRINTF("packet discarded");
            continue;
        }
        retry = 0;

        /* the switch puts the interface from which I have to receive */
        *src_interface = raw_eth.raw_msg.header.rx.src;
        *port = raw_eth.raw_msg.header.rx.port_type.field.port;
        *type = raw_eth.raw_msg.header.rx.port_type.field.type;
        /* payload */
        *payload = raw_eth.raw_msg.payload;
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function receives a raw message from a node
 * @param dev The axiom devive private data pointer
 * @param src_node_id Sender node identification
 * @param port port of the raw message
 * @param type type of the raw message
 * @param payload Data to receive
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_recv_raw(axiom_dev_t *dev, axiom_node_id_t *src_node_id,
        axiom_port_t *port, axiom_type_t *type, axiom_raw_payload_t *payload)
{
    axiom_routing_payload_t rt_message;
    axiom_raw_eth_t raw_eth;
    uint32_t axiom_msg_length;
    int ret, retry = 1;

    while (retry) {
        /* receive the length of the ethernet packet */
        ret = recv(((axiom_sim_node_args_t*)dev)->net->switch_fd,
                &axiom_msg_length, sizeof(axiom_msg_length), MSG_WAITALL);
        if (ret < 0) {
            return AXIOM_RET_ERROR;
        }

        axiom_msg_length = ntohl(axiom_msg_length);
        if (axiom_msg_length > sizeof(raw_eth)) {
            EPRINTF("too long message - len: %d", axiom_msg_length);
            return AXIOM_RET_ERROR;
        }

        /* receive ethernet packet */
        ret = recv(((axiom_sim_node_args_t*)dev)->net->switch_fd, &raw_eth,
                axiom_msg_length, MSG_WAITALL);
        if (ret != axiom_msg_length)
        {
            EPRINTF("unexpected length - expected: %d received: %d",
                    axiom_msg_length, ret);
            return AXIOM_RET_ERROR;
        }

        if (raw_eth.eth_hdr.type != htons(AXIOM_ETH_TYPE_RAW)) {
            retry = 1;
            DPRINTF("packet discarded");
            continue;
        }
        retry = 0;

        if (raw_eth.raw_msg.header.rx.port_type.field.port !=
                AXIOM_RAW_PORT_INIT)
        {
            continue;
        }

        memcpy(&rt_message, &raw_eth.raw_msg.payload, sizeof(rt_message));

        *port = raw_eth.raw_msg.header.rx.port_type.field.port;
        DPRINTF("routing: received on socket = %d for node %d: (%d,%d) [I'm node %d]",
                ((axiom_sim_node_args_t*)dev)->net->switch_fd,
                raw_eth.raw_msg.header.tx.dst, rt_message.node_id,
                rt_message.if_mask, axiom_get_node_id(dev));

        if (rt_message.command ==  AXIOM_RT_CMD_INFO)
        {
            /* it is local routing table, return the info to the caller */
            *src_node_id = raw_eth.raw_msg.header.rx.src;
            *payload = raw_eth.raw_msg.payload;
        }
        else if (rt_message.command == AXIOM_RT_CMD_END_INFO)
        {
            *payload = raw_eth.raw_msg.payload;
        }
        else if (rt_message.command ==  AXIOM_RT_CMD_RT_REPLY)
        {
            *payload = raw_eth.raw_msg.payload;
        }
    }

    return AXIOM_RET_OK;
}
