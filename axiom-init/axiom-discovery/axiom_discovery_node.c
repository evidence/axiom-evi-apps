/*!
 * \file axiom_discovery_node.c
 *
 * \version     v0.13
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the discovery and routing messages for Master and Slaves nodes.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "axiom_nic_packets.h"
#include "axiom_nic_discovery.h"
#include "axiom_nic_init.h"
#include "axiom_routing.h"
#include "axiom_discovery_protocol.h"
#include "../axiom-init.h"

static void notify_end_of_discovery();

static void
print_topology(axiom_node_id_t tpl[][AXIOM_INTERFACES_MAX],
        axiom_node_id_t last_node)
{
    int i, j;

    printf("\n************* Master computed Topology *******************\n");
    printf("Node");
    for (i = 0; i < AXIOM_INTERFACES_MAX; i++) {
        printf("\tIF%d", i);
    }
    printf("\n");

    for (i = 0; i < last_node; i++) {
        printf("%d", i);
        for (j = 0; j < AXIOM_INTERFACES_MAX; j++) {
            printf("\t%u", tpl[i][j]);
        }
        printf("\n");
    }
}

static void
print_routing_table(axiom_dev_t *dev, axiom_node_id_t node_id,
        axiom_node_id_t max_node_id)
{
    int i, j;
    uint8_t enabled_mask;

    printf("\nNode %d ROUTING TABLE\n", node_id);
    printf("Node");
    for (i = 0; i < AXIOM_INTERFACES_MAX; i++) {
        printf("\tIF%d", i);
    }
    printf("\n");

    for (i = 0; i <= max_node_id; i++) {
        printf("%d", i);

        axiom_get_routing(dev, i, &enabled_mask);

        for (j = 0; j < AXIOM_INTERFACES_MAX; j++) {
            if (enabled_mask & (uint8_t)(1 << j)) {
                printf("\t1");
            }
            else {
                printf("\t0");
            }
        }
        printf("\n");
    }
}

/* Master node code */
void
axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_if_id_t final_routing_table[AXIOM_NODES_MAX], int verbose)
{
    axiom_msg_id_t ret;
    axiom_node_id_t last_node = 0, master_id = AXIOM_INIT_MASTER_NODE;
    axiom_if_id_t routing_tables[AXIOM_NODES_MAX][AXIOM_NODES_MAX];

    IPRINTF(verbose, "MASTER: start discovery protocol");

    /* Discovery phase: discover the global topology */
    ret = axiom_master_node_discovery(dev, topology, master_id, &last_node);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("MASTER: axiom_master_node_discovery failed");
        return;
    }
    IPRINTF(verbose, "MASTER: end discovery protocol");

    IPRINTF(verbose, "MASTER: compute routing tables - first_node: %u"
    		    " last_node: %u", master_id, last_node);
    /* compute each node routing table */
    axiom_compute_routing_tables(topology, routing_tables,
            master_id, last_node);

    /* copy its routing table */
    memcpy(final_routing_table, routing_tables[master_id],
            sizeof(axiom_if_id_t)*AXIOM_NODES_MAX);

    IPRINTF(verbose, "MASTER: delivery routing tables");

    /* delivery of each node routing tables */
    ret = axiom_delivery_routing_tables(dev, routing_tables,
            master_id, last_node);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("MASTER: axiom_delivery_routing_tables failed");
        return;
    }
    IPRINTF(verbose, "MASTER: wait for all delivery reply");
    ret = axiom_wait_rt_received(dev, master_id, last_node);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("Master: axiom_wait_rt_received failed");
        return;
    }

    /* Say to all nodes to set the received routing table */
    ret = axiom_set_routing_table(dev, final_routing_table, 1);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("Master: axiom_set_routing_table failed");
        return;
    }

    IPRINTF(verbose, "MASTER: end");

    /* print the final topology */
    print_topology(topology, last_node);

    /* print local routing table */
    print_routing_table(dev, master_id, last_node - 1);

    /* notification end of discovery */
    notify_end_of_discovery();

}

/* Slave node code*/
void
axiom_discovery_slave(axiom_dev_t *dev,
        axiom_node_id_t first_src, void *first_payload,
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_if_id_t final_routing_table[AXIOM_NODES_MAX], int verbose)
{
    axiom_node_id_t node_id, max_node_id = 0;
    axiom_msg_id_t ret;

    IPRINTF(verbose, "SLAVE: start discovery protocol");

    /* Discovery phase: discover the intermediate topology */
    ret = axiom_slave_node_discovery(dev, topology, &node_id, first_src,
            (axiom_discovery_payload_t *)first_payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("SLAVE[%u]: axiom_slave_node_discovery failed", node_id);
        return;
    }
    IPRINTF(verbose, "SLAVE: end discovery protocol - ID assegned: %u",
            node_id);

    IPRINTF(verbose, "SLAVE[%u]: waiting routing table", node_id);
    /* Receive local routing table */
    ret = axiom_receive_routing_tables(dev, node_id,
            final_routing_table, &max_node_id);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("SLAVE[%u]: axiom_receive_routing_tables failed", node_id);
        return;
    }

    IPRINTF(verbose, "SLAVE[%u]: routing table received", node_id);

    /* Set local final routing table*/
    ret = axiom_set_routing_table(dev, final_routing_table, 0);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("SLAVE[%u]: axiom_set_routing_table failed", node_id);
        return;
    }

    IPRINTF(verbose, "SLAVE[%u]: routing table set", node_id);

    /* print local routing table */
    print_routing_table(dev, node_id, max_node_id);

    /* notification end of discovery */
    notify_end_of_discovery();

    IPRINTF(verbose, "SLAVE[%u]: end", node_id);
}

#define NOTIFY_SCRIPT_NAME "axiom_end_discovery.sh"
#define NOTIFY_SCRIPT ("/etc/" NOTIFY_SCRIPT_NAME)

static void
notify_end_of_discovery() {
    if (access(NOTIFY_SCRIPT,X_OK)==0) {
        pid_t pid=fork();
        if (pid==-1) {
            EPRINTF("fork() error");
            return;
        }
        if (pid==0) {
            // CHILD
            pid_t pid2=fork();
            if (pid2==-1) {
                EPRINTF("fork() 2 error");
                exit(EXIT_FAILURE);
            }
            if (pid2==0) {
                // CHILD of CHILD
                // exec notify script!!!
                execl(NOTIFY_SCRIPT,NOTIFY_SCRIPT_NAME,NULL);
                EPRINTF("execl() error");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        } else {
            int status;
            waitpid(pid,&status,0);
        }
    }
}
