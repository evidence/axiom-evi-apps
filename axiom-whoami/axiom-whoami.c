/*!
 * \file axiom-whoami.c
 *
 * \version     v0.9
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-whoami application.
 *
 * axiom-whoami prints the node id of the local axiom node
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
    axiom_node_id_t node_id;

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    printf ("I'm node %d\n", node_id);

    axiom_close(dev);

    return 0;
}
