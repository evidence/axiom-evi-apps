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

#include "dprintf.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_regs.h"

void print_nodeid(axiom_dev_t *dev)
{
    axiom_node_id_t nodeid;

    nodeid = axiom_get_node_id(dev);

    printf("\tnode id = %u\n\n", nodeid);
}

void print_ifnumber(axiom_dev_t *dev)
{
    axiom_err_t err;
    axiom_if_id_t if_number;

    err = axiom_get_if_number(dev, &if_number);
    if (err) {
        EPRINTF("err: %x if_number: %x", err, if_number);
    }

    printf("\tnumber of interfaces = %u\n\n", if_number);
}

void print_ifinfo(axiom_dev_t *dev, axiom_if_id_t if_id)
{
    axiom_err_t err;
    uint8_t if_features;

    err = axiom_get_if_info(dev, if_id, &if_features);
    if (err) {
        EPRINTF("err: %x if_features: %x", err, if_features);
    }

    printf("\tinterfaces[%u] status: 0x%02x\n", if_id, if_features);

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
    printf("\n");
}

void print_ifinfo_all(axiom_dev_t *dev)
{
    axiom_err_t err;
    axiom_if_id_t if_number;
    int i;

    err = axiom_get_if_number(dev, &if_number);
    if (err) {
        EPRINTF("err: %x if_number: %x", err, if_number);
    }

    for (i = 0; i < if_number; i++) {
        print_ifinfo(dev, i);
    }
}

void print_routing_table(axiom_dev_t *dev, int max_id)
{
    axiom_err_t err;
    uint8_t enabled_mask;
    int i, j;

    printf("\trouting table [0-%d]\n", max_id);
    printf("\t\tnode\tIF0\tIF1\tIF2\tIF3\n");

    for (i = 0; i <= max_id; i++) {
        err = axiom_get_routing(dev, i, &enabled_mask);
        if (err) {
            EPRINTF("err: %x enabled_mask: %x", err, enabled_mask);
            break;
        }

        printf("\t\t%d\t",i);

        for (j = 0; j < AXIOM_MAX_INTERFACES; j++) {
            int on = ((enabled_mask & (1 << j)) != 0);

            printf("%d\t", on);
        }

        printf("\n");
    }
    printf("\n");
}

void print_ni_status(axiom_dev_t *dev)
{
    uint32_t status;

    status = axiom_read_ni_status(dev);

    printf("\tstatus register = 0x%08x\n", status);

    /*TODO*/

    printf("\n");
}


void print_ni_control(axiom_dev_t *dev)
{
    uint32_t control;
    int loopback;

    control = axiom_read_ni_control(dev);

    printf("\tcontrol register = 0x%08x\n", control);

    loopback =  ((control & AXIOMREG_CONTROL_LOOPBACK) != 0);
    printf("\t\tLOOPBACK = %d\n\n", loopback);

    printf("\n");
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

    print_ifnumber(dev);

    print_ifinfo_all(dev);

    print_routing_table(dev, AXIOM_MAX_NODES);

    print_ni_status(dev);

    print_ni_control(dev);

    axiom_close(dev);

    return 0;
}
