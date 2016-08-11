/*!
 * \file axiom-recv.c
 *
 * \version     v0.7
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-recv application.
 *
 * axiom-recv receives AXIOM raw message
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
#include "dprintf.h"

static void
usage(void)
{
    printf("usage: axiom-recv [arguments]\n");
    printf("Receive AXIOM raw message\n\n");
    printf("Arguments:\n");
    printf("-p, --port       port   port used for receiving\n");
    printf("-o, --once              receive one message and exit\n");
    printf("-n, --noblocking        use no blocking receive\n");
    printf("-f, --flush             flush all messages previously received\n");
    printf("-s, --sleep      sec    second to sleep between two recv (only in"
                                    "noblocking)\n");
    printf("-v, --verbose           verbose\n");
    printf("-h, --help              print this help\n\n");
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_args_t axiom_args;
    axiom_msg_id_t recv_ret;
    axiom_node_id_t src_id, node_id;
    axiom_port_t port = 1, recv_port;
    int port_ok = 0, once = 0;
    axiom_type_t type;
    axiom_raw_payload_t payload;
    axiom_err_t err;
    float sleep_time = 1;
    int no_blocking = 0, verbose = 0, flush = 0;
    int msg_id = 0;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
            {"port", required_argument,  0, 'p'},
            {"once", no_argument,        0, 'o'},
            {"sleep", required_argument, 0, 's'},
            {"noblocking", no_argument,  0, 'n'},
            {"flush", no_argument,  0, 'f'},
            {"verbose", no_argument,     0, 'v'},
            {"help", no_argument,        0, 'h'},
            {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hop:s:nfv",
                         long_options, &long_index )) != -1) {
        switch (opt) {
            case 'o':
                once = 1;
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

    printf("[node %u] device opened - bind-flush: %d no-blocking: %d\n",
            node_id, flush, no_blocking);

    /* bind the current process on port */
    err = axiom_bind(dev, port);
    if (err != port) {
        EPRINTF("bind error");
        exit(-1);
    }

    printf("[node %u] receiving raw messages on port %u...\n", node_id, port);

    do {
        axiom_raw_payload_size_t payload_size = sizeof(payload);
        int i, avail;

        if (no_blocking) {
            avail = axiom_recv_raw_avail(dev);
            IPRINTF(verbose, "packets available: %d", avail);
            if (avail > 0) {
                printf("[node %u] packets available: %d \n", node_id, avail);
            }
        }

        /* receive a raw message from port*/
        recv_ret =  axiom_recv_raw(dev, &src_id, (axiom_port_t *)&recv_port,
                &type, &payload_size, &payload);
        if (!AXIOM_RET_IS_OK(recv_ret)) {
            if (no_blocking && (recv_ret == AXIOM_RET_NOTAVAIL)) {
                IPRINTF(verbose, "no packets available, waiting %f secs",
                        sleep_time);
                usleep(sleep_time * 1000000);
                continue;
            }
            EPRINTF("receive error");
            break;
        }

        msg_id++;

        printf("[node %u msg_id %d] message received on port %u\n", node_id,
                msg_id, recv_port);

        if (verbose) {
            if (type == AXIOM_TYPE_RAW_NEIGHBOUR) {
                printf("\t- local_interface = %u\n", src_id);
                printf("\t- type = %s\n", "NEIGHBOUR");
            } else if (type == AXIOM_TYPE_RAW_DATA) {
                printf("\t- source_node_id = %u\n", src_id);
                printf("\t- type = %s\n", "DATA");
            }

            printf("\t- payload_size = %u\n", payload_size);
            printf("\t- payload = ");
            for (i = 0; i < payload_size; i++)
                printf("0x%x ", payload.raw[i]);

            printf("\n");
        }

    } while (!once);

    axiom_close(dev);


    return 0;
}
