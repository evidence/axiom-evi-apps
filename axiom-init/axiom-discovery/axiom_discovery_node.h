#ifndef AXIOM_DISCOVERY_NODE_H
#define AXIOM_DISCOVERY_NODE_H

#include "axiom_discovery_protocol.h"

/*
 * @brief This function implements the master node code.
 * @param dev                   The axiom device private data pointer
 * @param topology              matrix memorizing the final network topology.
 * @param final_routing_table   final master node routing table
 * @param verbose               enable verbose output
 */
void
axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES], int verbose);

/*
 * @brief This function implements the slaves node code.
 * @param dev                   The axiom device private data pointer
 * @param first_src             interface where the first message is received
 * @param first_message         first message received
 * @param topology              matrix memorizing the intermediate nodes
 *                              topology.
 * @param final_routing_table   final node routing table
 * @param verbose               enable verbose output
 */
void
axiom_discovery_slave(axiom_dev_t *dev,
        axiom_node_id_t first_src, axiom_payload_t first_payload,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES], int verbose);

#endif /*! AXIOM_DISCOVERY_NODE_H*/
