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

#include "axiom_nic_types.h"
#include "axiom_node_code.h"

#define MASTER_PARAMETER  1

int main(int argc, char **argv)
{
    int master_slave;
    axiom_dev_t *dev = NULL;
    axiom_node_id_t topology[AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES];
    axiom_if_id_t routing_tables[AXIOM_NUM_NODES][AXIOM_NUM_NODES];
    axiom_if_id_t final_routing_table[AXIOM_NUM_NODES];

    /* first parameter: Master or slave */
    if (argc < 2) {
        printf("Parameter required: Master or Slave\n");
        exit(-1);
    }

    if (sscanf(argv[1], "%i", &master_slave) != 1) {
        perror("parameter is not an integer");
        exit(-1);
    }

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    /* bind the current process on port 0 */
    /* err = axiom_bind(dev, AXIOM_SMALL_PORT_DISCOVERY); */

    if (master_slave == MASTER_PARAMETER)
    {
        /* Master code */
        axiom_master_node_code(dev, topology, routing_tables, final_routing_table);
    }
    else
    {
        /* Slave code */
        axiom_slave_node_code(dev, topology, final_routing_table);
    }

    axiom_close(dev);

    return 0;
}
