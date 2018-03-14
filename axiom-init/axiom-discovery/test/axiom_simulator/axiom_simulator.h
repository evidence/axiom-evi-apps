/*!
 * \file axiom_simulator.h
 *
 * \version     v1.2
 *
 * This file contains AXIOM node network simulation structure and prototypes
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_SIMULATOR_h
#define AXIOM_SIMULATOR_h

#include "axiom_discovery_protocol.h"

/*! \brief Rotuing table value for not conneced node */
#define AXIOM_NULL_INTERFACE            255

typedef struct axiom_net axiom_net_t;

typedef struct node_args {
    pthread_t tid;
    int num_interfaces;
    axiom_node_id_t node_id;
    axiom_node_id_t node_topology[AXIOM_NODES_NUM][AXIOM_INTERFACES_NUM];
    int local_routing[AXIOM_NODES_NUM][AXIOM_INTERFACES_NUM];
    axiom_if_id_t final_routing_table[AXIOM_NODES_NUM];
    axiom_net_t *net;   /* net emulator status */
} axiom_sim_node_args_t;

typedef axiom_topology_t axiom_sim_topology_t;


void print_topology(axiom_sim_topology_t *tpl);
void print_local_routing_table(axiom_sim_node_args_t *nodes, int num_nodes);
void print_routing_tables (axiom_if_id_t rt[][AXIOM_NODES_NUM], int num_nodes);
void print_received_routing_table(axiom_sim_node_args_t *nodes, int num_nodes);
int start_nodes(axiom_sim_node_args_t *nodes, int num_nodes, int master_node,
                void *(*master_body)(void *), void *(*slave_body)(void *));
void wait_nodes(axiom_sim_node_args_t *nodes, int num_nodes);
void set_node_info(axiom_sim_node_args_t *node_args);
axiom_sim_node_args_t *get_node_info();

#endif /* !AXIOM_SIMULATOR_h */
