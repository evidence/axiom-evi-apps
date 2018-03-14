/*!
 * \file axiom_simulator.c
 *
 * \version     v1.2
 *
 * This file implements AXIOM node network simulation
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_simulator.h"
#include "axiom_routing.h"


pthread_key_t node_info_key;      /* thread private data */


void
print_topology(axiom_sim_topology_t *tpl)
{
    int i, j;

    printf("Node");
    for (i = 0; i < tpl->num_interfaces; i++) {
        printf("\tIF%d", i);
    }
    printf("\n");

    for (i = 0; i < tpl->num_nodes; i++) {
        printf("%d", i);
        for (j = 0; j < tpl->num_interfaces; j++) {
            printf("\t%u", tpl->topology[i][j]);
        }
        printf("\n");
    }
}

void
print_local_routing_table(axiom_sim_node_args_t *nodes, int num_nodes)
{
    int index, i, j;

    for (index = 0; index < num_nodes; index++) {
        printf("\nNode %d LOCAL ROUTING TABLE\n", nodes[index].node_id);
        printf("Node");
        for (i = 0; i < nodes[index].num_interfaces; i++) {
            printf("\tIF%d", i);
        }
        printf("\n");

        for (i = 0; i < num_nodes; i++) {
            printf("%d", i);
            for (j = 0; j < nodes[index].num_interfaces; j++) {
                printf("\t%u", nodes[index].local_routing[i][j]);
            }
            printf("\n");
        }
    }
}

void
print_routing_tables (axiom_if_id_t rt[][AXIOM_NODES_NUM], int num_nodes)
{
    int i, j;
    int comma = 0;

    for (i = 0; i < num_nodes; i++) {
        printf("\nNode %d ROUTING TABLE\n", i);
        for (j = 0; j < num_nodes; j++) {
            printf("\tNode%d", j);
        }

        printf("\n");
        printf("IF");
        for (j = 0; j < num_nodes; j++) {
            printf ("\t(");
            if (rt[i][j] & 0x01) {
                printf ("%d", 0);
                comma = 1;
            }
            if (rt[i][j] & 0x02) {
                if (comma == 1)
                    printf(",");
                else
                    comma = 1;
                printf ("%d", 1);
            }
            if (rt[i][j] & 0x04) {
                if (comma == 1)
                    printf(",");
                else
                    comma = 1;
                printf ("%d", 2);
            }
            if (rt[i][j] & 0x08) {
                if (comma == 1)
                    printf(",");
                else
                    comma = 1;
                printf ("%d", 3);
            }
            comma = 0;
            printf (")");
        }
        printf("\n");
    }
}

void
print_received_routing_table(axiom_sim_node_args_t *nodes, int num_nodes)
{
    int index, j;
    int comma = 0;

    for (index = 0; index < num_nodes; index++) {
        printf("\nNode %d RECEIVED ROUTING TABLE\n", nodes[index].node_id);
        for (j = 0; j < AXIOM_NODES_NUM; j++) {
            printf("\tNode%d", j);
        }

        printf("\n");
        printf("IF");
        for (j = 0; j < num_nodes; j++) {
            printf ("\t(");
            if (nodes[index].final_routing_table[j] & 0x01) {
                printf ("%d", 0);
                comma = 1;
            }
            if (nodes[index].final_routing_table[j] & 0x02) {
                if (comma == 1)
                    printf(",");
                else
                    comma = 1;
                printf ("%d", 1);
            }
            if (nodes[index].final_routing_table[j] & 0x04) {
                if (comma == 1)
                    printf(",");
                else
                    comma = 1;
                    printf ("%d", 2);
            }
            if (nodes[index].final_routing_table[j] & 0x08) {
                if (comma == 1)
                    printf(",");
                else
                    comma = 1;
                printf ("%d", 3);
            }

            comma = 0;
            printf (")");
        }
        printf("\n");
    }

}

int
start_nodes(axiom_sim_node_args_t *nodes, int num_nodes, int master_node,
        void *(*master_body)(void *), void *(*slave_body)(void *))
{
    int i, j, h, err;

    pthread_key_create(&node_info_key, NULL);

    for (i = 0; i < num_nodes; i++) {
        void *(*t_body)(void *);
        if (i != master_node) {
            t_body = slave_body;
        } else {
            t_body = master_body;
        }


        /* init the number of node interfaces */
        nodes[i].num_interfaces = AXIOM_INTERFACES_NUM;
        /* init the node local routing table */
        for (j = 0; j < num_nodes; j++) {
            for (h = 0; h < nodes[i].num_interfaces; h++) {
                nodes[i].local_routing[j][h] = 0;
            }
        }

        err = pthread_create(&nodes[i].tid, NULL, t_body, (void *)&nodes[i]);
        if (err) {
            EPRINTF("unable to create pthread err=%d", err);
            return err;
        }
    }

    return 0;
}

void
wait_nodes(axiom_sim_node_args_t *nodes, int num_nodes)
{
    int i;

    for (i = 0; i < num_nodes; i++) {
        pthread_join(nodes[i].tid, NULL);
    }

    pthread_key_delete(node_info_key);
}


void
set_node_info(axiom_sim_node_args_t *args)
{
    pthread_setspecific(node_info_key, (void *)args);
}

axiom_sim_node_args_t *
get_node_info()
{
    return (axiom_sim_node_args_t *)pthread_getspecific(node_info_key);
}

