/*!
 * \file axiom-recv.c
 *
 * \version     v0.8
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-recv application.
 *
 * axiom-recv receives AXIOM raw or long  message
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

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-recv [arguments]\n");
    printf("Receive AXIOM raw or long message\n\n");
    printf("Arguments:\n");
    printf("-t, --type       raw|long|all  message type to use [default: ALL]\n");
    printf("-p, --port       port          port used for receiving\n");
    printf("-o, --once                     receive one message and exit\n");
    printf("-n, --noblocking               use no blocking receive\n");
    printf("-f, --flush                    flush all messages previously received\n");
    printf("-s, --sleep      sec           second to sleep between two recv (only in"
                                           "noblocking)\n");
    printf("-v, --verbose                  verbose\n");
    printf("-h, --help                     print this help\n\n");
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_netperf_type_t np_type = AXIOM_RECV_DEF_TYPE;
    axiom_args_t axiom_args;
    axiom_node_id_t src_id, node_id;
    axiom_port_t port = 1, recv_port;
    int port_ok = 0, once = 0;
    axiom_type_t type;
    axiom_err_t err;
    float sleep_time = 1;
    int no_blocking = 0, flush = 0, timeout;
    int seq = 0;
    struct pollfd fds[2];
    int nfds = 0, raw_fd = -1, long_fd = -1;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
            {"port", required_argument,  0, 'p'},
            {"type", required_argument,  0, 't'},
            {"once", no_argument,        0, 'o'},
            {"sleep", required_argument, 0, 's'},
            {"noblocking", no_argument,  0, 'n'},
            {"flush", no_argument,  0, 'f'},
            {"verbose", no_argument,     0, 'v'},
            {"help", no_argument,        0, 'h'},
            {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hop:s:nft:v",
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
                } else if (strncmp(type_string, "all", 3) == 0) {
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

    err = axiom_get_fds(dev, &raw_fd, &long_fd, NULL);
    if (!AXIOM_RET_IS_OK(err)) {
        exit(-1);
    }

    if (np_type & AXNP_RAW) {
        printf("[node %u] receiving RAW messages on port %u...\n", node_id, port);
        fds[nfds].fd = raw_fd;
        fds[nfds].events = POLLIN;
        nfds++;
    }
    if (np_type & AXNP_LONG) {
        printf("[node %u] receiving LONG messages on port %u...\n", node_id, port);
        fds[nfds].fd = long_fd;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    if (no_blocking) {
        timeout = 0;
    } else {
        timeout = -1;
    }

    do {
        axiom_raw_payload_t raw_payload;
        axiom_long_payload_t long_payload;
        uint8_t *payload = NULL;
        int payload_size;
        int ifd, fds_ready;

        fds_ready = poll(fds, nfds, timeout);

        if (fds_ready < 0) {
            EPRINTF("poll error")
            break;
        } else if (fds_ready == 0) {
            IPRINTF(verbose, "no packets available, waiting %f secs",
                    sleep_time);
            usleep(sleep_time * 1000000);
            continue;
        }

        for (ifd = 0; ifd < nfds; ifd++) {
            if (fds[ifd].revents & (POLLERR | POLLHUP)) {
                EPRINTF("error");
                break;
            }

            if (fds[ifd].revents & POLLIN) {
                char msg_type_str[16];
                axiom_err_t recv_ret = -1;

                if (fds[ifd].fd == raw_fd) {
                    axiom_raw_payload_size_t raw_psize = sizeof(raw_payload);
                    /* receive a raw message from port*/
                    recv_ret =  axiom_recv_raw(dev, &src_id, (axiom_port_t *)&recv_port,
                            &type, &raw_psize, &raw_payload);
                    payload_size = raw_psize;
                    payload = ((uint8_t *)(&raw_payload));
                    strcpy(msg_type_str, "RAW");
                } else if (fds[ifd].fd == long_fd) {
                    axiom_long_payload_size_t long_psize = sizeof(long_payload);
                    /* receive a long message from port*/
                    recv_ret =  axiom_recv_long(dev, &src_id,
                            (axiom_port_t *)&recv_port, &long_psize, &long_payload);

                    type = AXIOM_TYPE_LONG_DATA;
                    payload_size = long_psize;
                    payload = ((uint8_t *)(&long_payload));
                    strcpy(msg_type_str, "LONG");
                }

                if (!AXIOM_RET_IS_OK(recv_ret)) {
                    EPRINTF("receive error");
                    break;
                }

                seq++;

                printf("[node %u seq %d] %s msg received - msg_id %d "
                        "src_id %d port %u payload_size %d \n",
                        node_id, seq, msg_type_str, recv_ret, src_id,
                        recv_port, payload_size);

                IPRINTF(verbose, "recv_ret: %d", recv_ret);

                if (verbose) {
                    int i;
                    if (type == AXIOM_TYPE_RAW_NEIGHBOUR) {
                        printf("\t- local_interface = %u\n", src_id);
                        printf("\t- type = %s\n", "RAW NEIGHBOUR");
                    } else if (type == AXIOM_TYPE_RAW_DATA) {
                        printf("\t- source_node_id = %u\n", src_id);
                        printf("\t- type = %s\n", "RAW DATA");
                    } else if (type == AXIOM_TYPE_LONG_DATA) {
                        printf("\t- source_node_id = %u\n", src_id);
                        printf("\t- type = %s\n", "LONG DATA");
                    }

                    printf("\t- payload_size = %u\n", payload_size);
                    printf("\t- payload = ");
                    for (i = 0; i < payload_size; i++)
                        printf("0x%x ", payload[i]);

                    printf("\n");
                }
            }
        }
    } while (!once);

    axiom_close(dev);


    return 0;
}
