/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.2
 * Last update: 2016-03-23
 *
 * This file tests the AXIOM INIT implementation
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#ifndef AXIOM_SIM
#define AXIOM_SIM   1
#endif
#include "axiom_node_code.h"
#include "axiom_simulator.h"
#include "axiom_net.h"

axiom_sim_node_args_t axiom_nodes[AXIOM_NUM_NODES];

/* Matrix memorizing the topolgy of the network
 * (a row for each node, a column for each interface)
 - topology[id_node1][id_iface1] = id_node2, id_iface means that
   id_node1 node, on its id_iface_1 is connected with id_node12 node
*/
/* axiom_node_id_t topology[NUMBER_OF_NODES][AXIOM_NUM_INTERFACES]; */
#ifdef EXAMPLE0
axiom_topology_t start_topology = {
    .topology = {
        { 2, 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 3, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 1, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 1, 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
axiom_topology_t end_test_topology = {
    .topology = {
        { 1, 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 3, 1, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
int end_test_local_routing_table [AXIOM_NUM_NODES][AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES] = {
        {{ 0, 0, 0, 0},  /* Node 0 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 1 */
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 2 */
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 3 */
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},},
};
#endif /* EXAMPLE0 */
#ifdef EXAMPLE1
axiom_topology_t start_topology = {
    .topology = {
        { 1, 2, 3, AXIOM_NULL_NODE},
        { 0, 6, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 4, 2, AXIOM_NULL_NODE},
        { 3, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 6, 4, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 1, 5, 7, AXIOM_NULL_NODE},
        { 6, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
axiom_topology_t end_test_topology = {
    .topology = {
        { 1, 6, 5, AXIOM_NULL_NODE},
        { 0, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 1, 3, 7, AXIOM_NULL_NODE},
        { 2, 4, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 5, 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 4, 6, AXIOM_NULL_NODE},
        { 0, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
int end_test_local_routing_table [AXIOM_NUM_NODES][AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES] = {
        {{ 0, 0, 0, 0},  /* Node 0 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 1 */
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 2 */
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 1, 0},},
        {{ 1, 0, 0, 0},  /* Node 3 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 4 */
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 5 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 0, 0, 1, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 6 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 7 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},},
};

#endif
#ifdef EXAMPLE2
axiom_topology_t start_topology = {
    .topology = {
        { 1, 2, 3, AXIOM_NULL_NODE},
        { 0, 4, 8, AXIOM_NULL_NODE},
        { 5, 0, 4, AXIOM_NULL_NODE},
        { 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, 1, 5, AXIOM_NULL_NODE},
        { 2, 4, 6, 7},
        { 8, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 5, 9, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 6, 1, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 7, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
axiom_topology_t end_test_topology = {
    .topology = {
        { 1, 3, 9, AXIOM_NULL_NODE},
        { 0, 2, 6, AXIOM_NULL_NODE},
        { 3, 1, 4, AXIOM_NULL_NODE},
        { 4, 0, 2, AXIOM_NULL_NODE},
        { 3, 2, 5, 7},
        { 6, 4, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 5, 1, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 4, 8, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 7, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
int end_test_local_routing_table [AXIOM_NUM_NODES][AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES] = {
        {{ 0, 0, 0, 0},  /* Node 0 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 1, 0},},
        {{ 1, 0, 0, 0},  /* Node 1 */
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 2 */
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},
        { 0, 1, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},
        { 0, 1, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 3 */
        { 0, 1, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 4 */
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},
        { 0, 0, 0, 1},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 5 */
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 6 */
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 7 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 8 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 9 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},},
};
#endif
#ifdef EXAMPLE3
axiom_topology_t start_topology = {
    .topology = {
        { 1, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 3, 3, 4},
        { 0, 4, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 1, 1, 4, 5},
        { 3, 1, 2, AXIOM_NULL_NODE},
        { 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
axiom_topology_t end_test_topology = {
    .topology = {
        { 1, 4, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 2, 2, 3},
        { 1, 1, 3, 5},
        { 2, 1, 4, AXIOM_NULL_NODE},
        { 0, 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
int end_test_local_routing_table [AXIOM_NUM_NODES][AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES] = {
        {{ 0, 0, 0, 0},  /* Node 0 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 1 */
        { 0, 0, 0, 0},
        { 0, 1, 1, 0},
        { 0, 0, 0, 1},
        { 1, 0, 0, 0},
        { 0, 1, 1, 0},},
        {{ 1, 1, 0, 0},  /* Node 2 */
        { 1, 1, 0, 0},
        { 0, 0, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},
        { 0, 0, 0, 1},},
        {{ 0, 1, 0, 0},  /* Node 3 */
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 0, 1, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 4 */
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 5 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},},
};
#endif
#ifdef EXAMPLE4
axiom_topology_t start_topology = {
    .topology = {
        { 1, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 4, 3, 3},
        { 4, 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 4, 1, 1, 5},
        { 3, 1, 2, AXIOM_NULL_NODE},
        { 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
axiom_topology_t end_test_topology = {
    .topology = {
        { 1, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 2, 3, 3},
        { 3, 1, 5, AXIOM_NULL_NODE},
        { 2, 1, 1, 4},
        { 3, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
int end_test_local_routing_table [AXIOM_NUM_NODES][AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES] = {
        {{ 0, 0, 0, 0},  /* Node 0 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 1 */
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 1, 1},
        { 0, 0, 1, 1},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 2 */
        { 0, 1, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 1, 0},},
        {{ 0, 1, 1, 0},  /* Node 3 */
        { 0, 1, 1, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 0, 0, 1},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 4 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 5 */
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},},
};
#endif
#ifdef EXAMPLE5
axiom_topology_t start_topology = {
    .topology = {
        { 1, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 1, 3, 4, AXIOM_NULL_NODE},
        { 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 4, 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
axiom_topology_t end_test_topology = {
    .topology = {
        { 1, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 0, 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 1, 3, 4, AXIOM_NULL_NODE},
        { 2, AXIOM_NULL_NODE, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 2, 5, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
        { 4, 0, AXIOM_NULL_NODE, AXIOM_NULL_NODE},
    },
    .num_nodes = AXIOM_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};
int end_test_local_routing_table [AXIOM_NUM_NODES][AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES] = {
        {{ 0, 0, 0, 0},  /* Node 0 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 1 */
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},},
        {{ 1, 0, 0, 0},  /* Node 2 */
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},
        { 0, 0, 1, 0},
        { 0, 0, 1, 0},},
        {{ 1, 0, 0, 0},  /* Node 3 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 4 */
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},
        { 0, 1, 0, 0},},
        {{ 0, 1, 0, 0},  /* Node 5 */
        { 0, 1, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 1, 0, 0, 0},
        { 0, 0, 0, 0},},
};
#endif

axiom_sim_topology_t final_topology = {
    .num_nodes = AXIOM_MAX_NUM_NODES,
    .num_interfaces = AXIOM_NUM_INTERFACES
};

axiom_if_id_t all_routing_tables[AXIOM_MAX_NUM_NODES][AXIOM_MAX_NUM_NODES];

extern axiom_node_id_t topology[AXIOM_MAX_NUM_NODES][AXIOM_NUM_INTERFACES];

void *master(void *args)
{
    axiom_sim_node_args_t *axiom_args = (axiom_sim_node_args_t *) args;
    axiom_dev_t *p_node_info;
 #if 0
    int i;
    char buf[1024];
#endif
    set_node_info(axiom_args);
    sleep(1);
    NDPRINTF("axiom_args %p get_node_info %p", axiom_args, get_node_info());

    /* get the pointer to the running thread 'axiom_nodes' entry */
    p_node_info = (axiom_dev_t *)get_node_info();

    axiom_master_node_code(p_node_info, axiom_args->node_topology, axiom_args->routing_tables,
					axiom_args->final_routing_table);

    memcpy(final_topology.topology, axiom_args->node_topology, sizeof(axiom_args->node_topology));

    memcpy(all_routing_tables, axiom_args->routing_tables, sizeof(axiom_args->routing_tables));

    DPRINTF("END");


    return 0;
}

void *slave(void *args)
{
    axiom_sim_node_args_t *axiom_args = (axiom_sim_node_args_t *) args;
    axiom_dev_t *p_node_info;

    set_node_info(axiom_args);
    sleep(1);
    NDPRINTF("axiom_args %p get_node_info %p", axiom_args, get_node_info());

    /* get the pointer to the running thread 'axiom_nodes' entry */
    p_node_info = (axiom_dev_t *)get_node_info();
    axiom_slave_node_code(p_node_info, axiom_args->node_topology, axiom_args->final_routing_table);

    return 0;
}

int main(int argc, char **argv)
{
    int err, cmp, cmp_ok;
    int node_index, i;

    printf("Start Topology\n");
    print_topology(&start_topology);

    /* initilize links between nodes */
    err = axiom_net_setup(axiom_nodes, &start_topology);
    if (err) {
        return err;
    }

    /* start nodes threads */
    start_nodes(axiom_nodes, AXIOM_NUM_NODES, AXIOM_MASTER_ID, master, slave);


    /* wait nodes */
    wait_nodes(axiom_nodes, AXIOM_NUM_NODES);

#if 0
    /*****************************************************************/
    /* Now this information are directly printed by master and slave */
    /*****************************************************************/

    printf("\n************* Final Topology *******************\n");
    print_topology(&final_topology);

    /* print local routing table after discovery */
    printf("\n************* Local routing table *******************\n");
    print_local_routing_table(axiom_nodes, AXIOM_NUM_NODES);
#endif

    /* print all routing tables computed by Master*/
    printf("\n************* MASTER GLOBAL routing Table *******************\n");
    print_routing_tables(all_routing_tables);


    /* print each node received routing tables after delivery */
    printf("\n************* Slave received routing table *******************\n");
    print_received_routing_table(axiom_nodes, AXIOM_NUM_NODES);


    /* ********************************************************************* */
    /* ************************* TEST RESULTS ****************************** */
    /* ********************************************************************* */
    printf("\n************* TEST RESULTS *******************");
    /* ************ TOPOLOGY *****************/
#if 0
    cmp = memcmp(&end_test_topology.topology, &final_topology.topology,
                                        sizeof(final_topology.topology));
#endif
    cmp = memcmp(&end_test_topology.topology, &final_topology.topology,
                  sizeof(uint8_t) * AXIOM_NUM_NODES * AXIOM_NUM_INTERFACES);
    if (cmp == 0)
    {
        printf("\nDiscovered Topology OK");
    }
    else
    {
        printf("\nDiscovered Topology ERROR");
    }

    /* ************ Final local routing tables *****************/
    cmp_ok = 0;
    for (node_index = 0; node_index < AXIOM_NUM_NODES; node_index++)
    {
        for (i = 0; i < AXIOM_NUM_NODES; i++)
        {
            if (axiom_nodes[i].node_id == node_index)
                break;
        }
        cmp = memcmp(axiom_nodes[i].local_routing,
                     end_test_local_routing_table[node_index],
                     sizeof(int) * AXIOM_NUM_NODES * AXIOM_NUM_INTERFACES);
        if (cmp != 0)
        {
            printf("\nNode %d routing table ERROR", node_index);
        }
        else
        {
            cmp_ok++;
        }
    }
    if (cmp_ok == AXIOM_NUM_NODES)
    {
        printf("\nNodes routing tables OK\n\n");
    }

    /* ********************************************************************* */
    /* *********************** END TEST RESULTS **************************** */
    /* ********************************************************************* */


    axiom_net_free(axiom_nodes, &start_topology);

    return 0;
}
