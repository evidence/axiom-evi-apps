/*!
 * \file axiom_routing.h
 *
 * \version     v0.15
 * \date        2016-05-03
 *
 * This file contains the defines and prototypes of axiom routing.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_ROUTING_H
#define AXIOM_ROUTING_H

#define AXIOM_NULL_RT_INTERFACE  0

/*!
 * \brief This function is executed by the Master node in order to compute the
 *        routing tables of all nodes.
 *
 * \param topology              Network topology discovered
 * \param[out] routing_tables   Routing table for all nodes
 * \param master_id             ID of master node
 * \param last_node             Last ID into network
 */
void
axiom_compute_routing_tables(axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_if_id_t routing_tables[][AXIOM_NODES_NUM],
        axiom_node_id_t master_id, axiom_node_id_t last_node);

/*!
 * \brief This function is executed by the Master node in order to
 *        delivery routing table using raw messages to all nodes.
 *
 * \param dev                   The axiom device private data pointer
 * \param routing_tables        Routing table for all nodes
 * \param master_id             ID of master node
 * \param last_node             Last ID into network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
axiom_err_t
axiom_delivery_routing_tables(axiom_dev_t *dev,
        axiom_if_id_t routing_tables[][AXIOM_NODES_NUM],
        axiom_node_id_t master_id, axiom_node_id_t last_node);

/*!
 * \brief This function is executed by the Master node in order to wait for all
 *        nodes routing table receiption confirmation.
 *
 * \param dev                   The axiom device private data pointer
 * \param master_id             ID of master node
 * \param last_node             Last ID into network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
axiom_err_t
axiom_wait_rt_received(axiom_dev_t *dev, axiom_node_id_t master_id,
        axiom_node_id_t last_node);

/*!
 * \brief This function is executed by each Slave nodes in order to receive its
 *        routing table
 *
 * \param dev                   The axiom device private data pointer
 * \param node_id               Node id of the caller
 * \param routing_table         Local routing table received
 * \param[out] max_node_id      Maximum node id on the network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
axiom_err_t
axiom_receive_routing_tables(axiom_dev_t *dev, axiom_node_id_t node_id,
        axiom_if_id_t routing_table[AXIOM_NODES_NUM],
        axiom_node_id_t *max_node_id);

/*!
 * \brief This function sets the routing table and forwards the commands to the
 *        neighbours.
 *
 * \param dev                   The axiom device private data pointer
 * \param routing_table         Routing table to set
 * \param master                Set to 1 in master node
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 *
 * This function is executed initially by the Master for communicating to the
 * neighbours to set the previously sent routing table.  Each slave node, when
 * it receives this message, sets the hardware routing table and forwards
 * message to its neighbours.
 */
axiom_err_t
axiom_set_routing_table(axiom_dev_t *dev,
        axiom_if_id_t routing_table[AXIOM_NODES_NUM], int master);

#endif /* !AXIOM_ROUTING_H */
