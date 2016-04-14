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
#include "axiom_nic_packets.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom-discovery/axiom_discovery_node.h"
#include "axiom-pong/axiom_pong.h"

int verbose = 0;

static void usage(void)
{
    printf("usage: axiom-init -n [master | slave]\n");
    printf("Start AXIOM node in slaves mode (or master if it is specified)\n\n");
    printf("-m, --master        start node as master\n");
    printf("-v, --verbose       verbose output\n");
    printf("-h, --help          print this help\n\n");
}


int main(int argc, char **argv)
{
    int master = 0, run = 1;
    axiom_dev_t *dev = NULL;
    axiom_node_id_t topology[AXIOM_MAX_NODES][AXIOM_MAX_INTERFACES];
    axiom_if_id_t final_routing_table[AXIOM_MAX_NODES];

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"master", no_argument, 0, 'm'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hvm",
                         long_options, &long_index )) != -1) {
        switch (opt) {
            case 'm':
                master = 1;
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

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    /* TODO: bind the current process on port 0 */
    /* err = axiom_bind(dev, AXIOM_SMALL_PORT_INIT); */

    if (master) {
        axiom_discovery_master(dev, topology, final_routing_table, verbose);
    }

    while(run) {
        axiom_node_id_t src;
        axiom_flag_t flag;
        axiom_init_cmd_t cmd;
        axiom_payload_t payload;
        axiom_err_t ret;

        ret = axiom_recv_small_init(dev, &src, &flag, &cmd, &payload);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("error receiving message");
            break;
        }
        switch (cmd) {
            case AXIOM_DSCV_CMD_REQ_ID:
                axiom_discovery_slave(dev, src, payload, topology,
                        final_routing_table, verbose);
                break;

            case AXIOM_PING:
                axiom_pong(dev, src, payload, verbose);
                break;

            default:
                EPRINTF("message discarded - cmd: %x", cmd);
        }
    }

    axiom_close(dev);

    return 0;
}
