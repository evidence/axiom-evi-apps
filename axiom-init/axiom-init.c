/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
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
#include <getopt.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_nic_types.h"
#include "axiom_node_code.h"

#define SLAVE_PARAMETER     0
#define MASTER_PARAMETER    1

int verbose = 0;

static void usage(void)
{
    printf("usage: axiom-init -n [master | slave]\n");
    printf("Start AXIOM discovery and routing algorithm in master or slaves mode\n\n");
    printf("-n, --node  [slave | master]   start node as master or as slave\n");
    printf("-v, --verbose                  verbose output\n\n");
    printf("-h, --help                     print this help\n\n");
}

int main(int argc, char **argv)
{
    int master_slave, ret;
    char *s_master = "master";
    char *s_slave = "slave";
    char node_str[30];
    axiom_dev_t *dev = NULL;

    int long_index =0;
    int opt = 0;
    axiom_node_id_t topology[AXIOM_MAX_NODES][AXIOM_MAX_INTERFACES];
    axiom_if_id_t routing_tables[AXIOM_MAX_NODES][AXIOM_MAX_NODES];
    axiom_if_id_t final_routing_table[AXIOM_MAX_NODES];
    static struct option long_options[] = {
        {"node", required_argument, 0, 'n'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hvn:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'n':
                sscanf(optarg, "%s", node_str);
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    if (strlen(node_str) == strlen(s_master))
    {
        ret = memcmp(node_str, s_master, strlen(node_str));
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
    else if (strlen(node_str) == strlen(s_slave))
    {
        ret = memcmp(node_str, s_slave, strlen(node_str));
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
        printf("Starting master node...\n");

        /* Master code */
        axiom_master_node_code(dev, topology, routing_tables, final_routing_table, verbose);

        printf("\nMaster node end\n");
    }
    else
    {
        printf("Starting slave node...\n");

        /* Slave code */
        axiom_slave_node_code(dev, topology, final_routing_table, verbose);

        printf("\nSlave node end\n");
    }

    axiom_close(dev);

    return 0;
}
