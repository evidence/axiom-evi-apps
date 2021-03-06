/*!
 * \file axiom_discovery_node.h
 *
 * \version     v1.2
 *
 * This file contains AXIOM node network simulation structure and prototypes
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_DISCOVERY_PROTOCOL_h
#define AXIOM_DISCOVERY_PROTOCOL_h

/*
 * @brief This function implements the master node code.
 * @param dev                   The axiom device private data pointer
 * @param topology              matrix memorizing the final network topology.
 * @param final_routing_table   final master node routing table
 * @param verbose               enable verbose output
 */
void
axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_if_id_t final_routing_table[AXIOM_NODES_NUM], int verbose);

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
        axiom_node_id_t first_src, axiom_raw_payload_t first_payload,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_if_id_t final_routing_table[AXIOM_NODES_NUM], int verbose);
#endif /*! AXIOM_DISCOVERY_PROTOCOL_h*/
