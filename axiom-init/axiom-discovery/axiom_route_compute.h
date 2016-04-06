/*
 * axiom_route_compute.h
 *
 * Version:     v0.3.1
 * Last update: 2016-03-22
 *
 * This file contains defines and prototypes for Master node
 * routing table computation for all AXIOM nodes
 *
 */
#ifndef ROUTE_COMPUTE_H
#define ROUTE_COMPUTE_H


#define AXIOM_NULL_RT_INTERFACE  0

/* define for the routing table memorization */
/* Example1: routing_tables[node_id][0] = AXIOM_IF_2 means:
            node_id is directly connected to node 0 through AXIOM_IF_2 */
/* Example2: routing_tables[node_id][5] = AXIOM_IF_1 | AXIOM_IF_3 means:
             node_id is directly connected to node 5 through AXIOM_IF_1
             and AXIOM_IF_3 (it is double linked to node 5)*/
#define AXIOM_IF_0      0xC0
#define AXIOM_IF_1      0x30
#define AXIOM_IF_2      0x0C
#define AXIOM_IF_3      0x03

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
