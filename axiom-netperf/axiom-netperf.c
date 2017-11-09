/*!
 * \file axiom-netperf.c
 *
 * \version     v0.14
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-netperf application.
 *
 * axiom-netperf estimates the throughput between two axiom nodes.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

#include "axiom-netperf.h"

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-netperf [arguments] -d server_id \n");
    printf("AXIOM netperf: estimate the throughput between this node and the\n");
    printf("               specified server_id\n");
    printf("Version: %s\n", AXIOM_API_VERSION_STR);
    printf("\n\n");
    printf("Arguments:\n");
    printf("client-mode:\n");
    printf("   -t, --type      raw|long|rdma|srdma message type to use [default: long]\n");
    printf("   -d, --dest      server_id           node id of axiom-netperf server\n");
    printf("   -p, --sport     server_port         port of axiom-netperf server[def. %d]\n",
            AXIOM_NETPERF_DEF_PORT);
    printf("   -P, --cport     client_port         port of axiom-netperf server[def. %d]\n",
            AXIOM_NETPERF_DEF_PORT);
    printf("   -l, --length    x[B|K|M|G]          bytes to send to the destination node\n");
    printf("                                       The suffix specifies the length unit\n");
    printf("   -L, --payload   size                payload size in bytes [default: "
            "raw - %d rdma - %d long - %d]\n",
            AXIOM_NETPERF_DEF_RAW_PSIZE, AXIOM_NETPERF_DEF_RDMA_PSIZE,
            AXIOM_NETPERF_DEF_LONG_PSIZE);
    printf("server-mode:\n");
    printf("   -s, --server                        enable server mode\n");
    printf("   -p, --port      server_port         server port [def. %d]\n",
            AXIOM_NETPERF_DEF_PORT);
    printf("-n, --num-threads  num_threads         number of threads [def. 1]\n");
    printf("-v, --verbose                          verbose output\n");
    printf("-V, --version                          print version\n");
    printf("-h, --help                             print this help\n\n");
}

static int
get_scale(char char_scale) {
    int scale;

    switch (char_scale) {
        case 'b': case 'B':
            scale = 0;
            break;

        case 'k': case 'K':
            scale = 10;
            break;

        case 'm': case 'M':
            scale = 20;
            break;

        case 'G': case 'g':
            scale = 30;
            break;

        default:
            scale = 0;
    }

    return scale;
}

int
main(int argc, char **argv)
{
    uint32_t data_length = AXIOM_NETPERF_DEF_DATA_LENGTH;
    int data_scale = AXIOM_NETPERF_DEF_DATA_SCALE;
    axnetperf_status_t s = {
        .dev = NULL,
        .server_id = AXIOM_NULL_NODE,
        .server_port = AXIOM_NETPERF_DEF_PORT,
        .client_port = AXIOM_NETPERF_DEF_PORT,
        .np_type = AXIOM_NETPERF_DEF_TYPE,
        .client.payload_size = 0,
        .num_threads = 1,
        .mutex = PTHREAD_MUTEX_INITIALIZER,
    };
    int i;
    axiom_err_t err;

    int long_index, opt;
    static struct option long_options[] = {
        {"type", required_argument, 0, 't'},
        {"dest", required_argument, 0, 'd'},
        {"sport", required_argument, 0, 'p'},
        {"cport", required_argument, 0, 'P'},
        {"length", required_argument, 0, 'l'},
        {"payload", required_argument, 0, 'L'},
        {"num_threads", required_argument, 0, 'n'},
        {"server", no_argument, 0, 's'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"svhd:l:L:p:P:t:n:V",
                         long_options, &long_index )) != -1) {
        char *type_string = NULL;
        char char_scale = AXIOM_NETPERF_DEF_CHAR_SCALE;

        switch (opt) {
            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &s.server_id) != 1) {
                    EPRINTF("wrong number of destination node");
                    usage();
                    exit(-1);
                }
                break;

            case 'p' :
                if (sscanf(optarg, "%" SCNu8, &s.server_port) != 1) {
                    EPRINTF("wrong number of destination port");
                    usage();
                    exit(-1);
                }
                break;

            case 'P' :
                if (sscanf(optarg, "%" SCNu8, &s.client_port) != 1) {
                    EPRINTF("wrong number of destination port");
                    usage();
                    exit(-1);
                }
                break;

            case 't':
                if ((sscanf(optarg, "%ms", &type_string) != 1) ||
                        type_string == NULL) {
                    EPRINTF("wrong message type");
                    usage();
                    exit(-1);
                }

                if (strncmp(type_string, "rdma", 4) == 0) {
                    s.np_type = AXNP_RDMA;
                    s.client.rdma_sync = 0;
                } else if (strncmp(type_string, "srdma", 4) == 0) {
                    s.np_type = AXNP_RDMA;
                    s.client.rdma_sync = 1;
                } else if (strncmp(type_string, "long", 4) == 0) {
                    s.np_type = AXNP_LONG;
                } else if (strncmp(type_string, "raw", 3) == 0) {
                    s.np_type = AXNP_RAW;
                } else {
                    EPRINTF("wrong message type");
                    free(type_string);
                    usage();
                    exit(-1);
                }

                free(type_string);
                break;

            case 'l' :
                if (sscanf(optarg, "%" SCNu32 "%c", &data_length, &char_scale) == 0) {
                    EPRINTF("wrong number of length");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                break;

            case 'L' :
                if (sscanf(optarg, "%zu", &s.client.payload_size) != 1) {
                    EPRINTF("wrong number of payload size");
                    usage();
                    exit(-1);
                }
                break;

            case 'n' :
                if (sscanf(optarg, "%u", &s.num_threads) != 1) {
                    EPRINTF("wrong number of threads");
                    usage();
                    exit(-1);
                }
                break;

            case 's':
                s.np_type = 0;
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

    /* open the axiom device */
    s.dev = axiom_open(NULL);
    if (s.dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    if (s.num_threads > AXNP_MAX_THREADS) {
        EPRINTF("Number of threads exceeds the maximum allowed [%d]",
                AXNP_MAX_THREADS);
        exit(-1);
    }

    s.state = AXN_STATE_NULL;


    /* server mode */
    if (s.np_type == 0) {
        err = axiom_bind(s.dev, s.server_port);
        if (err != s.server_port) {
            EPRINTF("axiom_bind error");
            goto err;
        }

        for (i = 0; i < s.num_threads; i++) {
            pthread_create(&s.threads[i], NULL, axnetperf_server, &s);
        }
    } else {
        err = axiom_bind(s.dev, s.client_port);
        if (err != s.client_port) {
            EPRINTF("axiom_bind error");
            goto err;
        }

        /* check if server_id parameter has been inserted */
        if (s.server_id == AXIOM_NULL_NODE) {
            EPRINTF("You must specify the destination node");
            goto err;
        }

        s.client.total_bytes = data_length << data_scale;

        for (i = 0; i < s.num_threads; i++) {
            pthread_create(&s.threads[i], NULL, axnetperf_client, &s);
        }
    }

    for (i = 0; i < s.num_threads; i++) {
        pthread_join(s.threads[i], NULL);
    }

err:
    axiom_close(s.dev);

    return 0;
}
