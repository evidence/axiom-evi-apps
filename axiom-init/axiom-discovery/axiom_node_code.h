/*
 * axiom_node_code.c
 *
 * Version:     v0.2
 * Last update: 2016-03-22
 *
 * This file defines prototypes of  AXIOM NIC init phase
 *
 */
#ifndef AXIOM_NODE_CODE_H
#define AXIOM_NODE_CODE_H

#include "axiom_discovery_protocol.h"

/*
 * @brief This function implements the master node code.
 * @param dev The axiom device private data pointer
 * * @param topology matrix memorizing the final network
 *        topology.
 * @param routing_tables matrix memorizing each nodes
 *        routing table, computed by Master at the end of the
 *        discovery phase (knowing the final network topology).
 * @param final_routing_table final master node routing table
 */
void axiom_master_node_code(axiom_dev_t *dev, axiom_node_id_t topology[][AXIOM_NUM_INTERFACES],
                      axiom_if_id_t routing_tables[][AXIOM_NUM_NODES],
                      axiom_if_id_t final_routing_table[AXIOM_NUM_NODES]);

/*
 * @brief This function implements the slaves node code.
 * @param dev The axiom device private data pointer
 * @param topology matrix memorizing the intermediate nodes
 *        topology.
 * @param final_routing_table final node routing table
 */
void axiom_slave_node_code(axiom_dev_t *dev, axiom_node_id_t topology[][AXIOM_NUM_INTERFACES],
                     axiom_if_id_t final_routing_table[AXIOM_NUM_NODES]);

#endif /*! AXIOM_NODE_CODE_H*/
