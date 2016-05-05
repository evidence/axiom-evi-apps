#ifndef ROUTE_COMPUTE_H
#define ROUTE_COMPUTE_H
/*!
 * \file axiom_route_compute.h
 *
 * \version     v0.4
 * \date        2016-05-03
 *
 * This file contains the defines and prototypes of routing table computation.
 */


#define AXIOM_NULL_RT_INTERFACE  0

/*
 * \brief This function is executed by the Master node in order to compute the
 *        routing tables of all nodes.
 *
 * \param topology              Network topology discovered
 * \param[out] routing_tables   Routing table for all nodes
 * \param total_nodes           Number of nodes into network
 */
void axiom_compute_routing_tables(
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
        axiom_node_id_t total_nodes);

#endif /* !ROUTE_COMPUTE_H */
