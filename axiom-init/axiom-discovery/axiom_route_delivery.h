#ifndef ROUTE_DELIVERY_H
#define ROUTE_DELIVERY_H
/*!
 * \file axiom_route_delivery.h
 *
 * \version     v0.4
 * \date        2016-05-03
 *
 * This file contains the defines and prototypes of routing table delivery.
 */

/*!
 * \brief This function is executed by the Master node in order to
 *        delivery routing table using raw messages to all nodes.
 *
 * \param dev                   The axiom device private data pointer
 * \param routing_tables        Routing table for all nodes
 * \param total_nodes           Number of nodes into network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
axiom_err_t
axiom_delivery_routing_tables(axiom_dev_t *dev,
        axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
        axiom_node_id_t total_nodes);

/*!
 * \brief This function is executed by the Master node in order to wait for all
 *        nodes routing table receiption confirmation.
 *
 * \param dev                   The axiom device private data pointer
 * \param total_nodes           Number of nodes into network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
axiom_err_t
axiom_wait_rt_received(axiom_dev_t *dev,axiom_node_id_t total_nodes);

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
        axiom_if_id_t routing_table[AXIOM_MAX_NODES],
        axiom_node_id_t *max_node_id);

#endif /* !ROUTE_DELIVERY_H */
