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
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_nic_types.h"
#include "axiom_node_code.h"

#define SLAVE_PARAMETER     0
#define MASTER_PARAMETER    1

static void usage(void)
{
    printf("usage: ./axiom-init [OPTION...]\n");
    printf("-n\t\tslave | master\t\tstart node as master or as slave\n");
    printf("-h, --help\t\t\t\tprint this help\n");
}

int main(int argc, char **argv)
{
    int master_slave, ret;
    char *s_master = "master";
    char *s_slave = "slave";
    int c;
    char cvalue[30];
    axiom_dev_t *dev = NULL;
    axiom_node_id_t topology[AXIOM_NUM_NODES][AXIOM_NUM_INTERFACES];
    axiom_if_id_t routing_tables[AXIOM_NUM_NODES][AXIOM_NUM_NODES];
    axiom_if_id_t final_routing_table[AXIOM_NUM_NODES];

    while ((c = getopt (argc, argv, "hn:")) != -1)
    {
        switch (c)
        {
            case 'h':
                usage();
                exit(-1);
            case 'n':
                sscanf(optarg, "%s", cvalue);
                break;
            case '?':
                if (optopt == 'n')
                {
                    usage();
                }
                else if (isprint (optopt))
                {
                    usage();
                }
                else
                {
                    usage();
                }
                return 1;
            default:
                exit(-1);
        }
    }

    if (strlen(cvalue) == strlen(s_master))
    {
        ret = memcmp(cvalue, s_master, strlen(cvalue));
        if (ret == 0)
        {
            master_slave = MASTER_PARAMETER;
        }
        else
        {
            usage();
            exit(-1);
        }
    }
    else
    {
        if (strlen(cvalue) == strlen(s_slave))
        {
            ret = memcmp(cvalue, s_slave, strlen(cvalue));
            if (ret == 0)
            {
                master_slave = SLAVE_PARAMETER;
            }
            else
            {
                usage();
                exit(-1);
            }
        }
        else
        {
            usage();
            exit(-1);
        }
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
