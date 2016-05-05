/*!
 * \file axiom-recv-small.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-recv-small application.
 *
 * axiom-recv-small receives AXIOM small raw message
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
    printf("usage: axiom-recv-small [arguments]\n");
    printf("Receive AXIOM small raw message\n\n");
    printf("Arguments:\n");
    printf("-p, --port  port     port used for receiving\n");
    printf("-o, --once           receive one message and exit\n");
    printf("-h, --help           print this help\n\n");
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t recv_ret;
    axiom_node_id_t src_id, node_id;
    axiom_port_t port = 1, recv_port;
    int port_ok = 0, once = 0;
    axiom_flag_t flag;
    axiom_payload_t payload;
    axiom_err_t err;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
            {"port", required_argument, 0, 'p'},
            {"once", no_argument,       0, 'o'},
            {"help", no_argument,       0, 'h'},
            {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hop:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'o':
                once = 1;
                break;

            case 'p' :
                if (sscanf(optarg, "%" SCNu8, &port) != 1)
                {
                    usage();
                    exit(-1);
                }
                port_ok = 1;
                break;

            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    if (port_ok == 1)
    {
        /* port arameter inserted */
        if ((port < 0) || (port > 255))
        {
            printf("Port not allowed [%u]; [0 < port < 256]\n", port);
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

    node_id = axiom_get_node_id(dev);

    /* bind the current process on port */
    err = axiom_bind(dev, port);
    if (err == AXIOM_RET_ERROR)
    {
        EPRINTF("bind error");
        exit(-1);
    }

    do {
        printf("[node %u] receiving small message on port %u...\n",
                node_id, port);

        /* receive a small message from port*/
        recv_ret =  axiom_recv_small(dev, &src_id, (axiom_port_t *)&recv_port,
                &flag, &payload);
        if (recv_ret == AXIOM_RET_ERROR)
        {
            EPRINTF("receive error");
            break;
        }

        printf("[node %u] message received on port %u\n", node_id, recv_port);
        if (flag & AXIOM_SMALL_FLAG_NEIGHBOUR) {
            printf("\t- local_interface = %u\n", src_id);
            printf("\t- flag = %s\n", "NEIGHBOUR");
        } else if (flag & AXIOM_SMALL_FLAG_DATA) {
            printf("\t- source_node_id = %u\n", src_id);
            printf("\t- flag = %s\n", "DATA");
        }
        printf("\t- payload = %u\n", payload);

    } while (!once);

    axiom_close(dev);


    return 0;
}
