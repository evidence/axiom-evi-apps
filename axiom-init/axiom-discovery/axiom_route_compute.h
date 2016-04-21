/*
 * This file contains defines and prototypes for Master node
 * routing table computation for all AXIOM nodes
 */
#ifndef ROUTE_COMPUTE_H
#define ROUTE_COMPUTE_H


#define AXIOM_NULL_RT_INTERFACE  0

/*
 * @brief This function is executed by the Master node in order to
 *        compute all nodes routing table.
 * @param topology The matrix with the network topology
 * @param routing_tables The matrix with the output NUM_NODES
 *        routing tables
 * @param number_of_total_nodes number of nodes into network
 * @return none
 */
void axiom_compute_routing_tables(axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
                                  axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
                                  axiom_node_id_t number_of_total_nodes);

#endif /* !ROUTE_COMPUTE_H */
