/*!
 * \file axiom-recv.c
 *
 * \version     v0.14
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-recv application.
 *
 * axiom-recv receives AXIOM raw or long  message
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#define AXIOM_RECV_DEF_TYPE             (AXNP_RAW | AXNP_LONG)
#define AXIOM_RECV_MAX_IOVEC            16

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-recv [arguments]\n");
    printf("Receive AXIOM raw or long message\n\n");
    printf("Arguments:\n");
    printf("-t, --type       raw|long|any  message type to use [default: ANY]\n");
    printf("-p, --port       port          port used for receiving\n");
    printf("-o, --once                     receive one message and exit\n");
    printf("-n, --noblocking               use no blocking receive\n");
    printf("-f, --flush                    flush all messages previously received\n");
    printf("-s, --sleep      sec           second to sleep between two recv (only in"
                                           "noblocking)\n");
    printf("-i, --iovec      num           use axiom API with iovec support\n");
    printf("                               (num = number of buffers)\n");
    printf("-m, --maxpayload max_payload   max payload to use\n");
    printf("-v, --verbose                  verbose\n");
    printf("-h, --help                     print this help\n\n");
}

static inline axiom_err_t
axrcv_recv(axiom_dev_t *dev, axiom_netperf_type_t np_type,
        axiom_node_id_t *src_id, axiom_port_t * port, axiom_type_t *type,
        size_t *payload_size, void *payload)
{
    axiom_err_t ret = AXIOM_RET_ERROR;

    if (np_type == (AXNP_RAW | AXNP_LONG)) {
        ret =  axiom_recv(dev, src_id, port, type, payload_size, payload);
    } else if (np_type == AXNP_RAW) {
        axiom_raw_payload_size_t raw_psize;
        if (*payload_size > AXIOM_RAW_PAYLOAD_MAX_SIZE)
            raw_psize = AXIOM_RAW_PAYLOAD_MAX_SIZE;
        else
            raw_psize = *payload_size;
        ret = axiom_recv_raw(dev, src_id, port, type, &raw_psize, payload);
        *payload_size = raw_psize;
    } else if (np_type == AXNP_LONG) {
        axiom_long_payload_size_t long_psize = *payload_size;
        ret = axiom_recv_long(dev, src_id, port, &long_psize, payload);
        *payload_size = long_psize;
        *type = AXIOM_TYPE_LONG_DATA;
    }

    return ret;
}

static inline axiom_err_t
axrcv_recv_iov(axiom_dev_t *dev, axiom_netperf_type_t np_type,
        axiom_node_id_t *src_id, axiom_port_t * port, axiom_type_t *type,
        size_t *payload_size, struct iovec *iov, int iovcnt)
{
    axiom_err_t ret = AXIOM_RET_ERROR;

    if (np_type == (AXNP_RAW | AXNP_LONG)) {
        ret =  axiom_recv_iov(dev, src_id, port, type, payload_size, iov,
                iovcnt);
    } else if (np_type == AXNP_RAW) {
        axiom_raw_payload_size_t raw_psize;
        if (*payload_size > AXIOM_RAW_PAYLOAD_MAX_SIZE)
            raw_psize = AXIOM_RAW_PAYLOAD_MAX_SIZE;
        else
            raw_psize = *payload_size;
        ret = axiom_recv_iov_raw(dev, src_id, port, type, &raw_psize, iov,
                iovcnt);
        *payload_size = raw_psize;
    } else if (np_type == AXNP_LONG) {
        axiom_long_payload_size_t long_psize = *payload_size;
        ret = axiom_recv_iov_long(dev, src_id, port, &long_psize, iov, iovcnt);
        *payload_size = long_psize;
        *type = AXIOM_TYPE_LONG_DATA;
    }

    return ret;
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_netperf_type_t np_type = AXIOM_RECV_DEF_TYPE;
    axiom_args_t axiom_args;
    axiom_node_id_t src_id, node_id;
    axiom_port_t port = 1, recv_port;
    axiom_long_payload_t payload;
    struct iovec recv_iov[AXIOM_RECV_MAX_IOVEC];
    int recv_iovcnt = 0, max_payload = sizeof(payload);
    int port_ok = 0, once = 0;
    axiom_type_t type;
    axiom_err_t err;
    float sleep_time = 1;
    int no_blocking = 0, flush = 0;
    int seq = 0;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
            {"port", required_argument,  0, 'p'},
            {"type", required_argument,  0, 't'},
            {"once", no_argument,        0, 'o'},
            {"sleep", required_argument, 0, 's'},
            {"noblocking", no_argument,  0, 'n'},
            {"flush", no_argument,       0, 'f'},
            {"iovec", required_argument, 0, 'i'},
            {"maxpayload", required_argument,  0, 'm'},
            {"verbose", no_argument,     0, 'v'},
            {"help", no_argument,        0, 'h'},
            {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hop:s:nft:m:i:v",
                         long_options, &long_index )) != -1) {
        char *type_string = NULL;

        switch (opt) {
            case 'o':
                once = 1;
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
                } else if (strncmp(type_string, "any", 3) == 0) {
                    np_type = AXNP_RAW | AXNP_LONG;
                } else {
                    EPRINTF("wrong message type");
                    free(type_string);
                    usage();
                    exit(-1);
                }

                free(type_string);
                break;

            case 'p':
                if (sscanf(optarg, "%" SCNu8, &port) != 1) {
                    usage();
                    exit(-1);
                }
                port_ok = 1;
                break;

            case 'n':
                no_blocking = 1;
                break;

            case 'f':
                flush = 1;
                break;

            case 's':
                if (sscanf(optarg, "%f", &sleep_time) != 1) {
                    usage();
                    exit(-1);
                }
                break;

            case 'i':
                if (sscanf(optarg, "%d", &recv_iovcnt) != 1) {
                    usage();
                    exit(-1);
                }
                break;

            case 'm':
                if (sscanf(optarg, "%d", &max_payload) != 1) {
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

    if (port_ok == 1) {
        /* port arameter inserted */
        if ((port < 0) || (port > 255)) {
            printf("Port not allowed [%u]; [0 < port < 256]\n", port);
            exit(-1);
        }
    }

    axiom_args.flags = 0;

    if (!flush)
        axiom_args.flags |= AXIOM_FLAG_NOFLUSH;

    if (no_blocking)
        axiom_args.flags |= AXIOM_FLAG_NOBLOCK;

    /* open the axiom device */
    dev = axiom_open(&axiom_args);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    if (recv_iovcnt < 0 || recv_iovcnt > AXIOM_RECV_MAX_IOVEC) {
        printf("Number of iovec not allowed [%d]; [0 <= iovcnt <= %d]\n",
                recv_iovcnt, AXIOM_RECV_MAX_IOVEC);
        exit(-1);
    }

    printf("[node %u] device opened - bind-flush: %d no-blocking: %d\n",
            node_id, flush, no_blocking);

    /* bind the current process on port */
    err = axiom_bind(dev, port);
    if (err != port) {
        EPRINTF("bind error");
        exit(-1);
    }

    if (np_type & AXNP_RAW) {
        printf("[node %u] receiving RAW messages on port %u...\n", node_id,
                port);
    }
    if (np_type & AXNP_LONG) {
        printf("[node %u] receiving LONG messages on port %u...\n", node_id,
                port);
    }

    if (max_payload < recv_iovcnt) {
        recv_iovcnt = 0;
    }

    if (recv_iovcnt > 0) {
        int i;
        size_t new_size = max_payload / recv_iovcnt, offset = 0;

        for (i = 0; (i < recv_iovcnt) && (offset < max_payload); i++) {
            recv_iov[i].iov_base = &payload.raw[offset];
            recv_iov[i].iov_len = new_size;

            offset += new_size;
        }

        recv_iov[recv_iovcnt - 1].iov_len = max_payload -
            (new_size * (recv_iovcnt - 1));
        printf("[node %u] using %d iovec - iovec.size: %zu, last_size: %zu\n",
                node_id, recv_iovcnt, new_size,
                recv_iov[recv_iovcnt - 1].iov_len);
    } else {
        printf("[node %u] using one buffer of %d bytes\n", node_id,
                max_payload);
    }

    do {
        axiom_err_t recv_ret = AXIOM_RET_ERROR;
        char msg_type_str[16];
        size_t payload_size = max_payload;

        if (recv_iovcnt > 0) {
            recv_ret =  axrcv_recv_iov(dev, np_type, &src_id, &recv_port,
                &type, &payload_size, recv_iov, recv_iovcnt);
        } else {
            recv_ret =  axrcv_recv(dev, np_type, &src_id, &recv_port,
                &type, &payload_size, &payload);
        }

        if (!AXIOM_RET_IS_OK(recv_ret)) {
            if (recv_ret == AXIOM_RET_NOTAVAIL) {
                IPRINTF(verbose, "no packets available, waiting %f secs",
                        sleep_time);
                usleep(sleep_time * 1000000);
                continue;
            }
            EPRINTF("receive error");
            break;
        }

        if (type == AXIOM_TYPE_RAW_DATA) {
            strcpy(msg_type_str, "RAW");
        } else if (type == AXIOM_TYPE_LONG_DATA) {
            strcpy(msg_type_str, "LONG");
        }

        seq++;

        printf("[node %u seq %d] %s msg received - msg_id %d "
                "src_id %d port %u payload_size %zu \n",
                node_id, seq, msg_type_str, recv_ret, src_id,
                recv_port, payload_size);

        IPRINTF(verbose, "recv_ret: %d", recv_ret);

        if (verbose) {
            int i;
            if (type == AXIOM_TYPE_RAW_NEIGHBOUR) {
                printf("\t- local_interface = %u\n", src_id);
                printf("\t- type = %s\n", "RAW NEIGHBOUR");
            } else {
                printf("\t- source_node_id = %u\n", src_id);
                printf("\t- type = %s\n", msg_type_str);
            }

            printf("\t- payload_size = %zu\n", payload_size);
            printf("\t- payload = ");
            for (i = 0; i < payload_size; i++)
                printf("0x%x ", payload.raw[i]);

            printf("\n");
        }
    } while (!once);

    axiom_close(dev);


    return 0;
}
