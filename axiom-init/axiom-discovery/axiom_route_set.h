/*
 * axiom_route_set.h
 *
 * Version:     v0.2
 * Last update: 2016-03-22
 *
 * This file defines AXIOM NIC final routing table set
 * function prototypes
 *
 */
#ifndef ROUTE_SET_H
#define ROUTE_SET_H

/*
 * @brief This function is executed by Master for communicating to
 *        each node to set the previously sent routing table.
 *        Each other node calls this function: when it receives
 *        this message, it sets its hardware routing table and it
 *        forwards message to its neighbours.
 * @param dev The axiom device private data pointer
 * @param final_routing_table node local routing table, containing
 *                  the routes to set into the hardware routing table
 */
axiom_msg_id_t axiom_set_routing_table(axiom_dev_t *dev,
                    axiom_if_id_t final_routing_table[AXIOM_NUM_NODES]);

#endif /* !ROUTE_SET_H */
