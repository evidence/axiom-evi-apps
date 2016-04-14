/*
 * axiom_discovery_protocol.h
 *
 * Version:     v0.3.1
 * Last update: 2016-03-22
 *
 * This file contains the AXIOM NIC discovery phase
 * defines and prototypes
 *
 */
#ifndef AXIOM_DSCV_PROTOCOL_H
#define AXIOM_DSCV_PROTOCOL_H

#include "dprintf.h"

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_sim_topology.h"

/* Master node ID */
#define AXIOM_MASTER_ID               0
/* Rotuing table value for not conneced node */
#define AXIOM_NULL_INTERFACE         255
/* Value returned by axiom_get_if_info() for a connected interface */
#define AXIOM_IF_CONNECTED            0x01

/* @brief Codify the mask to set into the routing table
 * @param dev The axiom device private data pointer
 * @param node_id identify the right entry of the routing table
 * @param interface_index index of the interface to codify
*/
uint8_t axiom_codify_routing_mask(axiom_dev_t *dev, axiom_node_id_t node_id,
                            uint8_t interface_index);
/*
 * @brief This function implements the Master node Discovery
 *        Algorithm.
 * @param dev The axiom device private data pointer
 * @param topology matrix memorizing the each node partial topology.
 *          When all nodes have executed this function, the topology
 *          matrix of Master contains the global topology of the
 *          network.
 * @param number_of_total_nodes number of nodes into the network; it
 *          is initialized at the end of the discovery phase
 */
int axiom_master_node_discovery(axiom_dev_t *dev,
                          axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
                          axiom_node_id_t *number_of_total_nodes);

/*
 * @brief This function implements the Other nodes Discovery Algorithm.
 * @param dev              The axiom device private data pointer
 * @param topology         matrix memorizing the each node partial topology.
 * @param my_node_id       final id of the node
 * @param first_interface  interface where the first message is received
 * @param first_message    first message received
 */
int axiom_slave_node_discovery (axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_node_id_t *my_node_id,
        axiom_if_id_t first_interface, axiom_payload_t first_msg);

#endif  /* !AXIOM_DSCV_PROTOCOL_H */
