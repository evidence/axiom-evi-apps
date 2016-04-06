/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.2
 * Last update: 2016-03-23
 *
 * This file tests the AXIOM INIT implementation
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "axiom_nic_api_user.h"
#include "dprintf.h"

void print_nodeid(axiom_dev_t *dev)
{
    axiom_node_id_t nodeid;

    nodeid = axiom_get_node_id(dev);

    printf("\tnode id = %u\n", nodeid);
}

void print_ifnumber(axiom_dev_t *dev)
{
    axiom_err_t err;
    axiom_if_id_t if_number;

    err = axiom_get_if_number(dev, &if_number);
    if (err) {
        EPRINTF("err: %x if_number: %x", err, if_number);
    }

    printf("\tnum of interfaces = %u\n", if_number);
}

void print_ifinfo(axiom_dev_t *dev)
{
    axiom_err_t err;
    axiom_if_id_t if_number;
    uint8_t if_features;
    int i;

    err = axiom_get_if_number(dev, &if_number);
    if (err) {
        EPRINTF("err: %x if_number: %x", err, if_number);
    }

    for (i = 0; i < if_number; i++) {
        printf("\tinterfaces[%u] status\n", if_id);

        err = axiom_get_if_info(dev, i, &if_features);
        if (err) {
            EPRINTF("err: %x if_features: %x", err, if_features);
        }

        if (if_features & AXIOMREG_IFINFO_CONNECTED) {
            printf("\t\tconnceted = 1\n");
        } else {
            printf("\t\tconnceted = 0\n");
        }

        if (if_features & AXIOMREG_IFINFO_TX) {
            printf("\t\ttx-enabled = 1\n");
        } else {
            printf("\t\ttx-enabled = 0\n");
        }

        if (if_features & AXIOMREG_IFINFO_RX) {
            printf("\t\trx-enabled = 1\n");
        } else {
            printf("\t\trx-enabled = 0\n");
        }
    }


    
}

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;

    printf("AXIOM NIC informations\n");
    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    print_nodeid(dev);


    axiom_close(dev);

    return 0;
}
