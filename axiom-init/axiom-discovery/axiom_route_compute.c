/*
 * axiom_route_compute.c
 *
 * Version:     v0.2
 * Last update: 2016-03-22
 *
 * This file implements Master node routing table
 * computation for all AXIOM nodes
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_discovery_protocol.h"
#include "axiom_route_compute.h"
#include "axiom_nic_api_user.h"


/* At the end of the discovery algorithm,  Master node
 * (which knows the entire network topology) performs a
 * similar-Dijkstra algorithm in order to compute the routing
 * tables of all the nodes of the network.
 * This algorithm assumes that each link between two nodes has
 * an unitary cost (the shortes path has to be computed) and it
 * starts from Mastr neighbours e than to their neighbours, etc...
 * In computing Y node routing table, each time an  Y neighbour
 * node or an Y neighbours connected node is found, the Master
 * immediately add this X node into the Y routing table; that is
 *  the first time the Master found an Y connected node it adds
 * the node into the routing table.
 * Since the nodes are added into the routing table from Master
 * to the increasing neighborhood level, the shortest path from
 * Y to each network node is computed.
 * This algorithm computes only a single path between the nodes.
*/


/* This function initializes a node routing table: the node
   is connected with no other node
   It initializes fisrt and second level node neighbours tables too*/
static void init_node_routing_table(axiom_if_id_t rt[][AXIOM_MAX_NODES],
                         axiom_if_id_t neighbour_table[][AXIOM_MAX_NODES])
{
    int i;

    for (i = 0; i < AXIOM_MAX_NODES; i++)
    {
        /* node routing table*/
        rt[0][i] = AXIOM_NULL_RT_INTERFACE;

        /* node first and second level neighbours talbes */
        neighbour_table[0][i] = AXIOM_NULL_NODE;
        neighbour_table[1][i] = AXIOM_NULL_NODE;
    }

}

/* This function computes the first level neighbours of
   'actual_node_id' node
    Return the number of founded neighbours */
static uint8_t compute_first_level_neighbours(
                       axiom_node_id_t actual_node_id,
                       axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
                       axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
                       axiom_node_id_t neighbour_table[][AXIOM_MAX_NODES])
{
    axiom_if_id_t interface_index, interface_index_to_set;
    axiom_node_id_t neighbour_id;
    uint8_t neighbours_counter;

    neighbours_counter = 0;

    for (interface_index = 0; interface_index < AXIOM_MAX_INTERFACES; interface_index++)
    {
        if (topology[actual_node_id][interface_index] != AXIOM_NULL_NODE)
        {
            /* first level neighbours--> memorize into routing table
               that the neighbour of actual_node_id is on interface
               'interface_index' */
            neighbour_id = topology[actual_node_id][interface_index];


            /* memorize the interface connected to neighbour_id */

            /* This commented solution doesn't manage the doble link
               from two nodes (Example: node 2 has IF0 and IF1
               directly conneted to node 5) */
            /* routing_tables[actual_node_id][neighbour_id] = interface_index; */

            interface_index_to_set = routing_tables[actual_node_id][neighbour_id];
            switch (interface_index)
            {
                case 0:
                    interface_index_to_set |= AXIOM_IF_0;
                    break;
                case 1:
                    interface_index_to_set |= AXIOM_IF_1;
                    break;
                case 2:
                    interface_index_to_set |= AXIOM_IF_2;
                    break;
                case 3:
                    interface_index_to_set |= AXIOM_IF_3;
                    break;

            }
            routing_tables[actual_node_id][neighbour_id] = interface_index_to_set;

            /* save node id of the neighbour */
            neighbour_table[0][neighbour_id] = neighbour_id;

            neighbours_counter++;
        }
    }

    return neighbours_counter;
}

/* This function is executed by the Master node in order to
 * compute all nodes routing table.*/
void axiom_compute_routing_tables(axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
                                  axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
                                  axiom_node_id_t number_of_total_nodes)
{
    /* ID of the node of of which the master is computing the routing table*/
    axiom_node_id_t actual_node_id;
    /* temp_rt[0] contains info about the n-level 'actual_node_id' neighbours*/
    /* temp_rt[1] contains info about the (n+1)-level 'actual_node_id' neighbours*/
    axiom_node_id_t neighbour_table[2][AXIOM_MAX_NODES];

    axiom_node_id_t node_index;
    axiom_if_id_t interface_index;
    axiom_node_id_t first_neighbour_id, second_neighbour_id;
    uint8_t neighbours_counter;

    /* for each node of the network */
    for (actual_node_id = 0; actual_node_id < number_of_total_nodes; actual_node_id++)
    {
        /* Init 'actual_node_id' node routing table and first and second
           level 'actual_node_id' neighbours tables */
        init_node_routing_table(&routing_tables[actual_node_id], neighbour_table);

        /* Compute the first level neighbours*/
        neighbours_counter = compute_first_level_neighbours(actual_node_id,
                                                            topology,
                                                            routing_tables,
                                                            neighbour_table);
       do
       {
            /* Compute the second level neighbours*/
            for (node_index = 0; node_index < number_of_total_nodes; node_index++)
            {
                if (neighbour_table[0][node_index] != AXIOM_NULL_NODE)
                {
                    first_neighbour_id = neighbour_table[0][node_index];

                    /* compute the neighbours of 'neighbour_id' node */
                    for (interface_index = 0; interface_index < AXIOM_MAX_INTERFACES; interface_index++)
                    {
                        if ((topology[first_neighbour_id][interface_index] != AXIOM_NULL_NODE) &&
                            (topology[first_neighbour_id][interface_index] != actual_node_id))
                        {
                            second_neighbour_id = topology[first_neighbour_id][interface_index];

                            if (routing_tables[actual_node_id][second_neighbour_id] == AXIOM_NULL_RT_INTERFACE)
                            {
                                /* memorize that 'second_neighbour_id' node can be reached
                                   through the interace connected with 'first_neighbour_id'*/
                                routing_tables[actual_node_id][second_neighbour_id] =
                                        routing_tables[actual_node_id][first_neighbour_id];

                                /* save node id of the second level neighbour */
                                neighbour_table[1][second_neighbour_id] = second_neighbour_id;

                                neighbours_counter++;
                            }
                        }
                    }

                }
            }

            /* the second level neighbours become the next first level neighbours in the algorithm */
            memcpy(&neighbour_table[0], &neighbour_table[1], AXIOM_MAX_NODES*sizeof(axiom_node_id_t));

        } while (neighbours_counter < number_of_total_nodes - 1);
        /* exit from cycle when 'actual_node_id' routing table has been completed with all the
            AXIOM_MAX_NODES - 1 nodes info */
    }
    return;
}






























