/*!
 * \file axiom-send.c
 *
 * \version     v0.8
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-send application.
 *
 * axiom-send sends AXIOM raw or long message to a specified remote node.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#define AXIOM_RECV_DEF_TYPE             AXNP_RAW

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-send [arguments] -d dest   payload (list of bytes)\n");
    printf("Send AXIOM raw or long message to specified dest (dest can be remote node\n");
    printf("or local interface, if you send a message to neighbour [-n])\n\n");
    printf("Arguments:\n");
    printf("-t, --type      raw|long  message type to use [default: RAW]\n");
    printf("-d, --dest      dest      dest node id or local if (TO_NEIGHBOUR) \n");
    printf("-r, --repeat    rep       repeat rep times the payload specified\n");
    printf("-p, --port      port      port used for sending this message\n");
    printf("-n, --neighbour           send message to neighbour (dest is used\n");
    printf("                          to specify the local interface)\n");
    printf("-c, --count     count     send the same message multiple times [def: 1]\n");
    printf("-s, --sleep     sec       second to sleep between two send \n");
    printf("-v, --verbose             verbose\n");
    printf("-h, --help                print this help\n\n");
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_netperf_type_t np_type = AXIOM_RECV_DEF_TYPE;
    axiom_err_t send_ret;
    axiom_port_t port = 1;
    axiom_node_id_t node_id, dst_id;
    axiom_type_t type = AXIOM_TYPE_RAW_DATA;
    axiom_long_payload_t payload;
    int payload_size = 0;
    int count = 1, repeat = 0, port_ok = 0, dst_ok = 0, to_neighbour = 0;
    int msg_id, i;
    float sleep_time = 0;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"type", required_argument,  0, 't'},
        {"repeat", required_argument, 0, 'r'},
        {"port", required_argument, 0, 'p'},
        {"count", required_argument, 0, 'c'},
        {"sleep", required_argument, 0, 's'},
        {"neighbour", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"ht:p:d:nr:c:s:v",
                         long_options, &long_index )) != -1) {
        char *type_string = NULL;

        switch (opt) {
            case 'p':
                if (sscanf(optarg, "%" SCNu8, &port) != 1) {
                    EPRINTF("wrong port");
                    usage();
                    exit(-1);
                }
                port_ok = 1;
                break;

            case 't':
                if ((sscanf(optarg, "%ms", &type_string) != 1) ||
                        type_string == NULL) {
                    EPRINTF("wrong message type");
                    usage();
                    exit(-1);
                }

                if (strncmp(type_string, "long", 4) == 0) {
                    np_type = AXNP_LONG;
                } else if (strncmp(type_string, "raw", 3) == 0) {
                    np_type = AXNP_RAW;
                } else {
                    EPRINTF("wrong message type");
                    free(type_string);
                    usage();
                    exit(-1);
                }

                free(type_string);
                break;

            case 'd':
                if (sscanf(optarg, "%" SCNu8, &dst_id) != 1) {
                    EPRINTF("wrong destination");
                    usage();
                    exit(-1);
                }
                dst_ok = 1;
                break;

            case 'n':
                to_neighbour = 1;
                break;

            case 'r':
                if (sscanf(optarg, "%d", &repeat) != 1) {
                    EPRINTF("wrong repeat parameter");
                    usage();
                    exit(-1);
                }
                break;

            case 'c':
                if (sscanf(optarg, "%d", &count) != 1) {
                    EPRINTF("wrong count parameter");
                    usage();
                    exit(-1);
                }
                break;

            case 's':
                if (sscanf(optarg, "%f", &sleep_time) != 1) {
                    EPRINTF("wrong sleep time parameter");
                    usage();
                    exit(-1);
                }
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

    /* check port parameter */
    if (port_ok == 1) {
        if ((port < 0) || (port > 7)) {
            printf("Port not allowed [%u]; [0 <= port <= 7]\n", port);
            exit(-1);
        }
    }

    /* check neighbour parameter */
    if (to_neighbour && (np_type != AXNP_RAW)) {
        printf("message to neighbour is available only with RAW type");
        exit(-1);
    }

    /* check destination node parameter */
    if (dst_ok == 1) {
        /* port arameter inserted */
        if ((dst_id < 0) || (dst_id > 255)) {
            printf("Destination node id not allowed [%u]; [0 <= dst_id < 256]\n", dst_id);
            exit(-1);
        }
    }
    else {
        usage();
        exit(-1);
    }

    /* read payload list */
    for (i = 0; (i < (argc - optind)) && (i < sizeof(payload)); i++) {
        if (sscanf(argv[i + optind], "%" SCNu8, &payload.raw[i]) != 1) {
            EPRINTF("wrong payload byte [index = %d]", i);
            exit(-1);
        }
        payload_size++;
    }

    if (payload_size == 0) {
        EPRINTF("payload empty");
        exit(-1);
    }

    if (repeat) {
        int orig_psize = payload_size;

        for (i = 1; i < repeat &&
                (payload_size + orig_psize <= sizeof(payload)); i++){
            memcpy(&payload.raw[payload_size], &payload.raw[0], orig_psize);
            payload_size += orig_psize;
        }
    }

    /* check neighbour parameter */
    if (to_neighbour == 1) {
        type = AXIOM_TYPE_RAW_NEIGHBOUR;
    }

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    /* bind the current process on port */
#if 0
    if (port_ok == 1) {
        err = axiom_bind(dev, port);
    }
#endif

    if (np_type == AXNP_RAW) {
        if (payload_size > AXIOM_RAW_PAYLOAD_MAX_SIZE)
            payload_size = AXIOM_RAW_PAYLOAD_MAX_SIZE;
        printf("[node %u] sending %d RAW messages...\n", node_id, count);
    } else if (np_type == AXNP_LONG) {
        if (payload_size > AXIOM_LONG_PAYLOAD_MAX_SIZE)
            payload_size = AXIOM_LONG_PAYLOAD_MAX_SIZE;
        type = AXIOM_TYPE_LONG_DATA;
        printf("[node %u] sending %d LONG messages...\n", node_id, count);
    }

    for (msg_id = 0; msg_id < count; msg_id++) {
        /* send a raw message*/
        if (np_type == AXNP_RAW) {
            axiom_raw_payload_size_t raw_psize = payload_size;
            send_ret =  axiom_send_raw(dev, (axiom_node_id_t)dst_id,
                            (axiom_port_t)port, type, raw_psize, &payload);
        } else if (np_type == AXNP_LONG) {
            axiom_long_payload_size_t long_psize = payload_size;
            send_ret =  axiom_send_long(dev, (axiom_node_id_t)dst_id,
                            (axiom_port_t)port, long_psize, &payload);
        }

        if (!AXIOM_RET_IS_OK(send_ret)) {
            EPRINTF("send error");
            goto err;
        }

        printf("[node %u msg-id %d] message sent - payload_size %d dest_id %d "
                "port %u\n",
                node_id, msg_id + 1, payload_size, dst_id, port);

        if (verbose) {
            if (type == AXIOM_TYPE_RAW_NEIGHBOUR) {
                printf("\t- local_interface = %u\n", dst_id);
                printf("\t- type = %s\n", "NEIGHBOUR");
            } else if (type == AXIOM_TYPE_RAW_DATA) {
                printf("\t- destination_node_id = %u\n", dst_id);
                printf("\t- type = %s\n", "DATA");
            } else if (type == AXIOM_TYPE_LONG_DATA) {
                printf("\t- destination_node_id = %u\n", dst_id);
                printf("\t- type = %s\n", "LONG DATA");
            }

            printf("\t- payload_size = %u\n", payload_size);
            printf("\t- payload = ");
            for (i = 0; i < payload_size; i++)
                printf("0x%x ", payload.raw[i]);

            printf("\n");
        }

        usleep(sleep_time * 1000000);
    }

err:
    axiom_close(dev);


    return 0;
}
