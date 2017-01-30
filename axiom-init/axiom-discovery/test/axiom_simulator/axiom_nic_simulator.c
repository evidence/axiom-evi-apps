/*!
 * \file axiom_nic_simulator.c
 *
 * \version     v0.11
 *
 * Axiom session management.
 * This file contains the AXIOM NIC API simulation
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_discovery.h"
#include "axiom_nic_routing.h"
#include "axiom_simulator.h"
#include "axiom_net.h"


/*
 * @brief This function returns the local node id.
 * @param dev The axiom devive private data pointer
 * return Returns the local node id.
 */
axiom_node_id_t
axiom_get_node_id(axiom_dev_t *dev)
{
    return ((axiom_sim_node_args_t*)dev)->node_id;
}

/*
 * @brief This function sets the id of a local node.
 * @param dev The axiom devive private data pointer
 * @param node_id The _id assigned to this node.
 */
void
axiom_set_node_id(axiom_dev_t *dev, axiom_node_id_t node_id)
{
    ((axiom_sim_node_args_t*)dev)->node_id = node_id;
}

/*
 * @brief This function reads the number of interfaces which are present on a
 * node.
 * @param dev The axiom devive private data pointer
 * @param if_number The number of the node interfaces
 * return Returns ...
 */
axiom_err_t
axiom_get_if_number(axiom_dev_t *dev, axiom_if_id_t* if_number)
{
    *if_number =  ((axiom_sim_node_args_t*)dev)->num_interfaces;

    return AXIOM_RET_OK; /* always return OK */
}

/*
 * @brief This function reads the features of an interface.
 * @param dev The axiom devive private data pointer
 * @param if_number The number of the node interface
 * @param if_features The features of the node interface
 * return Returns ...
 */
axiom_err_t
axiom_get_if_info(axiom_dev_t *dev, axiom_if_id_t if_number,
        uint8_t* if_features)
{
    if (if_number >= AXIOM_INTERFACES_MAX) {
        return AXIOM_RET_ERROR;
    }

    *if_features = 0;

    if (axiom_net_connect_status(dev, if_number)) {
        /* Exist a connection (a socket descriptor) on the 'if_number'
         * interface of the node managed by the running thread */
        *if_features = AXIOM_IF_CONNECTED;
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function sets the routing table of a local node.
 * @param dev The axiom devive private data pointer
 * @param node_id Remote connected node id
 * @param enabled_mask bit mask interface
 */
axiom_err_t
axiom_set_routing(axiom_dev_t *dev, axiom_node_id_t node_id,
        uint8_t enabled_mask)
{
    uint8_t i;

    for (i = 0; i < AXIOM_INTERFACES_MAX; i++) {
        if (enabled_mask & (uint8_t)(1 << i)) {
            ((axiom_sim_node_args_t*)dev)->local_routing[node_id][i] = 1;
        } else {
            ((axiom_sim_node_args_t*)dev)->local_routing[node_id][i] = 0;
        }
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function gets the routing table of a local node.
 * @param dev The axiom devive private data pointer
 * @param node_id Remote connected node id
 * @param enabled_mask bit mask interface
 */
axiom_err_t
axiom_get_routing(axiom_dev_t *dev, axiom_node_id_t node_id,
        uint8_t *enabled_mask)
{
    uint8_t i;

    *enabled_mask = 0;

    for (i = 0; i < AXIOM_INTERFACES_MAX; i++) {
        if (((axiom_sim_node_args_t*)dev)->local_routing[node_id][i] == 1) {
            *enabled_mask |= (uint8_t)(1 << i);
        }
    }

    return AXIOM_RET_OK;
}


/*
 * @brief This function sends raw data to a remote node.
 * @param dev The axiom devive private data pointer
 * @param dst_id The remote node id that will receive the raw data or local
 *               interface that will send the raw data
 * @param port port of the raw message
 * @param type type of the raw message
 * @param payload data to be sent
 * @return Returns a unique positive message id on success, -1 otherwise.
 * XXX: the return type is unsigned!
 */
axiom_msg_id_t
axiom_send_raw(axiom_dev_t *dev, axiom_node_id_t dst_id,
        axiom_port_t port, axiom_type_t type, axiom_raw_payload_t *payload)
{
    axiom_msg_id_t ret = AXIOM_RET_ERROR;

    if (type == AXIOM_TYPE_RAW_NEIGHBOUR) {
        ret = axiom_net_send_raw_neighbour(dev, (axiom_if_id_t)dst_id, port,
                type, payload);
    } else {
        ret = axiom_net_send_raw(dev, dst_id, port, type, payload);
    }

    return ret;
}

/*
 * @brief This function receives raw data.
 * @param dev The axiom devive private data pointer
 * @param src_id The source node id that sent the raw data or local
 *               interface that received the raw data
 * @param port port of the raw message
 * @param type type of the raw message
 * @param payload data received
 * @return Returns a unique positive message id on success, -1 otherwise.
 * XXX: the return type is unsigned!
 */
axiom_msg_id_t
axiom_recv_raw(axiom_dev_t *dev, axiom_node_id_t *src_id,
        axiom_port_t *port, axiom_type_t *type, axiom_raw_payload_t *payload)
{
    axiom_msg_id_t ret = AXIOM_RET_ERROR;

    if (*type == AXIOM_TYPE_RAW_NEIGHBOUR) {
        /* discovery and set routing neighbour messages management */
        ret = axiom_net_recv_raw_neighbour(dev, src_id, port, type, payload);
    } else {
        /* delivery raw messages management */
        ret = axiom_net_recv_raw(dev, src_id, port, type, payload);
    }

    return ret;
}
