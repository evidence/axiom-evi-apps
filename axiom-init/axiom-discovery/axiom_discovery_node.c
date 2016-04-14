/*
 * axiom_discovery_node.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-22
 *
 * This file implements AXIOM NIC init phase of
 * Master and Slaves nodes
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "axiom_discovery_node.h"
#include "axiom_nic_discovery.h"
#include "axiom_route_compute.h"
#include "axiom_route_delivery.h"
#include "axiom_route_set.h"

static
void print_topology(axiom_node_id_t tpl[][AXIOM_MAX_INTERFACES],
                           axiom_node_id_t number_of_total_nodes)
{
    int i, j;

    printf("\n************* Master computed Topology *******************\n");
    printf("Node");
    for (i = 0; i < AXIOM_MAX_INTERFACES; i++) {
        printf("\tIF%d", i);
    }
    printf("\n");

    for (i = 0; i < number_of_total_nodes; i++) {
        printf("%d", i);
        for (j = 0; j < AXIOM_MAX_INTERFACES; j++) {
            printf("\t%u", tpl[i][j]);
        }
        printf("\n");
    }
}

static
void print_my_routing_table(axiom_dev_t *dev,
                            axiom_node_id_t my_node_id,
                            axiom_node_id_t max_node_id)
{
    int i, j;
    uint8_t enabled_mask;

    printf("\nNode %d ROUTING TABLE\n", my_node_id);
    printf("Node");
    for (i = 0; i < AXIOM_MAX_INTERFACES; i++) {
        printf("\tIF%d", i);
    }
    printf("\n");

    for (i = 0; i <= max_node_id; i++)
    {
        printf("%d", i);

        axiom_get_routing(dev, i, &enabled_mask);

        for (j = 0; j < AXIOM_MAX_INTERFACES; j++)
        {
            if (enabled_mask & (uint8_t)(1 << j))
            {
                printf("\t1");
            }
            else
            {
                printf("\t0");
            }
        }
        printf("\n");
    }
}

/* Master node code */
void axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES], int verbose)
{
    axiom_msg_id_t ret;
    axiom_node_id_t number_of_total_nodes = 0;
    axiom_if_id_t routing_tables[AXIOM_MAX_NODES][AXIOM_MAX_NODES];

    IPRINTF(verbose, "MASTER: start discovery protocol");

    /* Discovery phase: discover the global topology */
    ret = axiom_master_node_discovery(dev, topology, &number_of_total_nodes);

    IPRINTF(verbose, "MASTER: end discovery protocol");

    if (ret == AXIOM_RET_OK)
    {
        IPRINTF(verbose, "MASTER: compute routing tables");
        /* compute each node routing table */
        axiom_compute_routing_tables(topology, routing_tables, number_of_total_nodes);

        /* copy its routing table */
        memcpy(final_routing_table, routing_tables[0], sizeof(axiom_if_id_t)*AXIOM_MAX_NODES);

        IPRINTF(verbose, "MASTER: delivery routing tables");

        /* delivery of each node routing tables */
        ret = axiom_delivery_routing_tables(dev, routing_tables, number_of_total_nodes);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("MASTER: axiom_delivery_routing_tables failed");
        }
        else
        {

            IPRINTF(verbose, "MASTER: wait for all delivery reply");
            ret = axiom_wait_rt_received(dev, number_of_total_nodes);
            if (ret == AXIOM_RET_ERROR)
            {
                EPRINTF("Master: axiom_wait_rt_received failed");
            }
            else
            {
                /* Say to all nodes to set the received routing table */
                ret = axiom_set_routing_table(dev, final_routing_table);
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("Master: axiom_set_routing_table failed");
                }
            }
        }
    }

    IPRINTF(verbose, "MASTER: end");

    /* print the final topology */
    print_topology(topology, number_of_total_nodes);

    /* print my routing table */
    print_my_routing_table(dev, AXIOM_MASTER_ID, number_of_total_nodes-1);
}

/* Slave node code*/
void axiom_discovery_slave(axiom_dev_t *dev,
        axiom_node_id_t first_src, axiom_payload_t first_payload,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES],
        int verbose)
{
    axiom_node_id_t my_node_id, max_node_id = 0;
    axiom_msg_id_t ret;

    IPRINTF(verbose, "SLAVE: start discovery protocol");

    /* Discovery phase: discover the intermediate topology */
    ret = axiom_slave_node_discovery(dev, topology, &my_node_id, first_src,
            first_payload);

    IPRINTF(verbose, "SLAVE: end discovery protocol - ID assegned: %u", my_node_id);

    if (ret == AXIOM_RET_OK)
    {
        IPRINTF(verbose, "SLAVE[%u]: waiting routing table", my_node_id);
        /* Receive my routing table */
        ret = axiom_receive_routing_tables(dev, my_node_id,
                                           final_routing_table, &max_node_id);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("SLAVE[%u]: axiom_receive_routing_tables failed", my_node_id);
            return;
        }

        IPRINTF(verbose, "SLAVE[%u]: routing table received", my_node_id);

        /* Set my final routing table*/
        ret = axiom_set_routing_table(dev, final_routing_table);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("SLAVE[%u]: axiom_set_routing_table failed", my_node_id);
            return;
        }

        IPRINTF(verbose, "SLAVE[%u]: routing table set", my_node_id);

        /* print my routing table */
        print_my_routing_table(dev, my_node_id, max_node_id);

    }

    IPRINTF(verbose, "SLAVE[%u]: end", my_node_id);
}
