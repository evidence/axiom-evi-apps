/*!
 * \file axiom-init.c
 *
 * \version     v0.15
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-init deamon.
 *
 * axiom-init starts an axiom node in slave or master mode and handles all
 * control messagges received on port 0.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <pthread.h>
#include <ctype.h>
#include <inttypes.h>
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
    printf("Start AXIOM node in slaves mode (or master if it is specified)\n");
    printf("Version: %s\n", AXIOM_API_VERSION_STR);
    printf("\n\n");
    printf("Arguments:\n");
    printf("-m, --master           start node as master\n");
    printf("-n, --nodeid    id     set node id\n");
    printf("-r, --routing   file   load routing table from file (each row (X) must contain the interface to reach node X)\n");
    printf("-s, --save      file   save routing table to file (after discovery)\n");
    printf("-v, --verbose          verbose output\n");
    printf("-V, --version          print version\n");
    printf("-h, --help             print this help\n\n");
}


int
axiom_rt_from_file(axiom_dev_t *dev, char *filename)
{
    axiom_if_id_t routing_table[AXIOM_NODES_NUM];
    FILE *file = NULL;
    char *line = NULL;
    size_t len = 0;
    int line_count = 0;

    memset(routing_table, 0, sizeof(routing_table));

    file = fopen(filename, "r");
    if (file == NULL)  {
        printf("File not exist\n");
        return -1;
    }

    while ((getline(&line, &len, file)) != -1) {
        line_count++;
        if (line_count > AXIOM_NODES_NUM) {
            printf("The routing table contains more than %d nodes\n", AXIOM_NODES_NUM);
            free(line);
            return -1;
        }

        long val = strtol(line, NULL, 10);
        if ((val >= 0) || (val <= AXIOM_INTERFACES_MAX))
            routing_table[line_count - 1] = (1 << val);
    }

    free(line);
    fclose(file);

    /* set final routing table */
    for (int nid = 0; nid < AXIOM_NODES_NUM; nid++) {
        axiom_set_routing(dev, nid, routing_table[nid]);
    }

    return 0;
}

int
axiom_rt_to_file(axiom_dev_t *dev, char *filename)
{
    axiom_if_id_t routing_table[AXIOM_NODES_NUM];
    FILE *file = NULL;

    memset(routing_table, 0, sizeof(routing_table));

    /* get final routing table */
    for (int nid = 0; nid < AXIOM_NODES_NUM; nid++) {
        axiom_get_routing(dev, nid, routing_table+nid);
    }

    file = fopen(filename, "w");
    if (file == NULL)  {
        printf("Can not open file for writing\n");
        return -1;
    }

    for (int nid = 0; nid < AXIOM_NODES_NUM; nid++) {
        int val=__builtin_ctzl(routing_table[nid]);
        fprintf(file,"%d\n",val);
    }

    fclose(file);

    return 0;
}

int
main(int argc, char **argv)
{
    int master = 0, run = 1, set_nodeid = 0, set_rt = 0, save_rt=0;
    char rt_filename[1024];
    char rt_save_filename[1024];
    axiom_dev_t *dev = NULL;
    axiom_args_t axiom_args;
    axiom_node_id_t topology[AXIOM_NODES_NUM][AXIOM_INTERFACES_NUM];
    axiom_node_id_t node_id = 0;
    axiom_if_id_t final_routing_table[AXIOM_NODES_NUM];
    axiom_err_t ret;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"master", no_argument, 0, 'm'},
        {"nodeid", required_argument, 0, 'n'},
        {"routing", required_argument, 0, 'r'},
        {"save", required_argument, 0, 's'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv,"hvmn:r:s:V",
                         long_options, &long_index )) != -1) {
        switch (opt) {
            case 'm':
                master = 1;
                break;
            case 'n':
                if (sscanf(optarg, "%" SCNu8, &node_id) != 1) {
                    EPRINTF("wrong node ID");
                    usage();
                    exit(-1);
                }
                set_nodeid = 1;
                break;
            case 'r':
                if (snprintf(rt_filename, sizeof(rt_filename) - 1, "%s",
                            optarg) < 0) {
                    EPRINTF("wrong filename");
                    usage();
                    exit(-1);
                }
                set_rt = 1;
                break;
            case 's':
                if (snprintf(rt_save_filename, sizeof(rt_save_filename) - 1, "%s",
                            optarg) < 0) {
                    EPRINTF("wrong filename");
                    usage();
                    exit(-1);
                }
                save_rt = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'V':
                printf("Version: %s\n", AXIOM_API_VERSION_STR);
                exit(0);
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

    /* bind the current process on port 0 */
    ret = axiom_bind(dev, AXIOM_RAW_PORT_INIT);
    if (ret != AXIOM_RAW_PORT_INIT) {
        EPRINTF("error binding port");
        exit(-1);
    }

    axiom_spawn_init();
    axiom_allocator_l1_init();

    if (set_nodeid) {
        axiom_set_node_id(dev, node_id);
    }

    if (master) {
        axiom_discovery_master(dev, topology, final_routing_table, verbose);
        if (save_rt) {
            if (axiom_rt_to_file(dev, rt_filename)) {
                axiom_close(dev);
                exit(-1);
            }
        }
    } else if (set_rt) {
        if (axiom_rt_from_file(dev, rt_filename)) {
            axiom_close(dev);
            exit(-1);
        }
        if (save_rt) {
            if (axiom_rt_to_file(dev, rt_filename)) {
                axiom_close(dev);
                exit(-1);
            }
        }
    }

    while(run) {
        axiom_node_id_t src;
        axiom_type_t type;
        axiom_init_cmd_t cmd;
        axiom_long_payload_t payload;
        size_t payload_size = sizeof(payload);

        ret = axiom_recv_init(dev, &src, &type, &cmd, &payload_size,
                &payload);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("error receiving message");
            break;
        }
        switch (cmd) {
            case AXIOM_DSCV_CMD_REQ_ID:
                axiom_discovery_slave(dev, src, &payload, topology,
                        final_routing_table, verbose);
                if (save_rt) {
                    if (axiom_rt_to_file(dev, rt_filename)) {
                        axiom_close(dev);
                        exit(-1);
                    }
                }
                break;

            case AXIOM_CMD_PING:
                axiom_pong(dev, src, &payload, verbose);
                break;

            case AXIOM_CMD_TRACEROUTE:
                axiom_traceroute_reply(dev, src, &payload, verbose);
                break;

            case AXIOM_CMD_SPAWN_REQ:
                axiom_spawn_req(dev, src, payload_size, &payload, verbose);
                break;

            case AXIOM_CMD_SESSION_REQ:
                axiom_session(dev, src, payload_size, &payload, verbose);
                break;

            case AXIOM_CMD_ALLOC:
            case AXIOM_CMD_ALLOC_APPID:
            case AXIOM_CMD_ALLOC_RELEASE:
                axiom_allocator_l1(dev, src, payload_size, &payload, verbose);
                break;

            default:
                EPRINTF("message discarded - cmd: 0x%x", cmd);
        }
    }

    axiom_close(dev);

    return 0;
}
