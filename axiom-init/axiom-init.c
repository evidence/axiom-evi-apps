/*!
 * \file axiom-init.c
 *
 * \version     v0.6
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-init deamon.
 *
 * axiom-init starts an axiom node in slave or master mode and handles all
 * control messagges received on port 0.
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
#include "axiom-init.h"

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-init [arguments]\n");
    printf("Start AXIOM node in slaves mode (or master if it is specified)\n\n");
    printf("Arguments:\n");
    printf("-m, --master        start node as master\n");
    printf("-v, --verbose       verbose output\n");
    printf("-h, --help          print this help\n\n");
}

int
main(int argc, char **argv)
{
    int master = 0, run = 1;
    axiom_dev_t *dev = NULL;
    axiom_args_t axiom_args;
    axiom_node_id_t topology[AXIOM_NODES_MAX][AXIOM_INTERFACES_MAX];
    axiom_if_id_t final_routing_table[AXIOM_NODES_MAX];
    axiom_err_t ret;

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

    /* avoid the flush of previous packets */
    axiom_args.flags = AXIOM_FLAG_NOFLUSH;

    /* open the axiom device */
    dev = axiom_open(&axiom_args);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    /* TODO: bind the current process on port 0 */
    ret = axiom_bind(dev, AXIOM_RAW_PORT_INIT);
    if (ret != AXIOM_RET_OK) {
        EPRINTF("error binding port");
        exit(-1);
    }

    if (master) {
        axiom_discovery_master(dev, topology, final_routing_table, verbose);
    }

    while(run) {
        axiom_node_id_t src;
        axiom_type_t type;
        axiom_init_cmd_t cmd;
        axiom_init_payload_t payload;
        axiom_raw_payload_size_t payload_size = sizeof(payload);

        ret = axiom_recv_raw_init(dev, &src, &type, &cmd, &payload_size,
                &payload);
        if (ret != AXIOM_RET_OK) {
            EPRINTF("error receiving message");
            break;
        }
        switch (cmd) {
            case AXIOM_DSCV_CMD_REQ_ID:
                axiom_discovery_slave(dev, src, &payload, topology,
                        final_routing_table, verbose);
                break;

            case AXIOM_CMD_PING:
                axiom_pong(dev, src, &payload, verbose);
                break;

            case AXIOM_CMD_TRACEROUTE:
                axiom_traceroute_reply(dev, src, &payload, verbose);
                break;

            case AXIOM_CMD_NETPERF: case AXIOM_CMD_NETPERF_START:
                axiom_netperf_reply(dev, src, payload_size, &payload, verbose);
                break;

            default:
                EPRINTF("message discarded - cmd: 0x%x", cmd);
        }
    }

    axiom_close(dev);

    return 0;
}
