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

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_node_id_t my_node_id;

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    my_node_id = axiom_get_node_id(dev);

    printf ("I'm node %d\n", my_node_id);

    axiom_close(dev);

    return 0;
}
