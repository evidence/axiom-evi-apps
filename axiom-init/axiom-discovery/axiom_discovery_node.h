/*
 * axiom_discovery_node.h
 *
 * Version:     v0.3.1
 * Last update: 2016-04-08
 *
 * This file defines prototypes of  AXIOM NIC init phase
 *
 */
#ifndef AXIOM_DISCOVERY_NODE_H
#define AXIOM_DISCOVERY_NODE_H

#include "axiom_discovery_protocol.h"

/*
 * @brief This function implements the master node code.
 * @param dev                   The axiom device private data pointer
 * @param topology              matrix memorizing the final network topology.
 * @param routing_tables        matrix memorizing each nodes routing table,
 *                              computed by Master at the end of the discovery
 *                              phase (knowing the final network topology).
 * @param final_routing_table   final master node routing table
 * @param verbose               enable verbose output
 */
void axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES], int verbose);

/*
 * @brief This function implements the slaves node code.
 * @param dev                   The axiom device private data pointer
 * @param topology              matrix memorizing the intermediate nodes
 *                              topology.
 * @param final_routing_table   final node routing table
 * @param first_message         first message received
 * @param first_interface       interface where the first message is received
 * @param verbose               enable verbose output
 */
void axiom_discovery_slave(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES],
        axiom_payload_t first_msg, axiom_if_id_t first_interface,
        int verbose);

#endif /*! AXIOM_DISCOVERY_NODE_H*/
