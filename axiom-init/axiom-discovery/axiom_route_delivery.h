/*
 * axiom_route_delivery.c
 *
 * Version:     v0.2
 * Last update: 2016-03-22
 *
 * This file defines prototypes for routing tables delivery
 *
 */
#ifndef ROUTE_DELIVERY_H
#define ROUTE_DELIVERY_H

/*
 * @brief This function is executed by the Master node in order to
 *        delivery all nodes routing table using raw messages.
 * @param routing_tables The matrix with the NUM_NODES routing tables
 * @param number_of_total_nodes number of nodes into the network
 * @return XXX
 */
axiom_msg_id_t axiom_delivery_routing_tables(axiom_dev_t *dev,
                          axiom_if_id_t routing_tables[][AXIOM_NUM_NODES],
                          axiom_node_id_t number_of_total_nodes);

/*
 * @brief This function is executed by each non-Master node in order
 *        to receive its routing table
 * @param my_node_id id of the node that calls this function for
 *                   receiving its final routing table
 * @param routing_tables local routing table to receive
 * @param max_node_id maximu node id of the network
 * @return XXX
 */
axiom_err_t axiom_receive_routing_tables(axiom_dev_t *dev,
                            axiom_node_id_t my_node_id,
                            axiom_if_id_t routing_tables[AXIOM_NUM_NODES],
                            axiom_node_id_t *max_node_id);

#endif /* !ROUTE_DELIVERY_H */
