/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-23
 *
 * This file implements the axiom-send-small application
 *
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

static void usage(void)
{
    printf("usage: ./axiom-send-small [[-p port] [-n] | [-h]] -d dest -l payload\n");
    printf("Send AXIOM small raw message\n\n");
    printf("-p, --port      port     port used for sending\n");
    printf("-d, --dest      dest     dest node id or local if (TO_NEIGHBOUR) \n");
    printf("-n, --neighbour          send to neighbour\n");
    printf("-l, --payload   payload  message to send (uint32_t)\n");
    printf("-h, --help               print this help\n\n");
}

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t recv_ret, my_node_id;
    axiom_port_t port = 1;
    axiom_node_id_t dst_id;
    int port_ok = 0, dst_ok = 0, payload_ok = 0, to_neighbour = 0;
    axiom_flag_t flag = AXIOM_SMALL_FLAG_DATA;
    axiom_payload_t payload;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {"payload", required_argument, 0, 'l'},
        {"neighbour", no_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"hp:d:nl:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'p' :
                if (sscanf(optarg, "%" SCNu8, &port) != 1)
                {
                    EPRINTF("wrong port");
                    usage();
                    exit(-1);
                }
                port_ok = 1;
                break;

            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &dst_id) != 1)
                {
                    EPRINTF("wrong destination");
                    usage();
                    exit(-1);
                }
                dst_ok = 1;
                break;

            case 'n' :
                to_neighbour = 1;
                break;

            case 'l' :
                if (sscanf(optarg, "%" SCNu32, &payload) != 1)
                {
                    EPRINTF("wrong payload");
                    usage();
                    exit(-1);
                }
                payload_ok = 1;
                break;

            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    /* check port parameter */
    if (port_ok == 1)
    {
        /* port arameter inserted */
        if ((port < 0) || (port > 255))
        {
            printf("Port not allowed [%u]; [0 < port < 256]\n", port);
            exit(-1);
        }
    }

    /* check destination node parameter */
    if (dst_ok == 1)
    {
        /* port arameter inserted */
        if ((dst_id < 0) || (dst_id > 255))
        {
            printf("Destination node id not allowed [%u]; [0 <= dst_id < 256]\n", dst_id);
            exit(-1);
        }
    }
    else
    {
        usage();
        exit(-1);
    }

    /* check payload parameter */
    if (payload_ok != 1)
    {
        EPRINTF("payload required");
        exit(-1);
    }

    /* check neighbour parameter */
    if (to_neighbour == 1)
    {
        flag = AXIOM_SMALL_FLAG_NEIGHBOUR;
    }



    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    my_node_id = axiom_get_node_id(dev);

    /* bind the current process on port */
#if 0
    if (port_ok == 1) {
        err = axiom_bind(dev, port);
    }
#endif

    printf("[node %u] sending small message...\n", my_node_id);
    /* send a small message*/
    recv_ret =  axiom_send_small(dev, (axiom_node_id_t)dst_id,
                                        (axiom_port_t)port, flag,
                                        (axiom_payload_t*)&payload);
    if (recv_ret == AXIOM_RET_ERROR)
    {
           EPRINTF("receive error");
           goto err;
    }

    printf("[node %u] message sent to port %u\n", my_node_id, port);
    if (flag & AXIOM_SMALL_FLAG_NEIGHBOUR) {
        printf("\t- local_interface = %u\n", dst_id);
        printf("\t- flag = %s\n", "NEIGHBOUR");
    } else if (flag & AXIOM_SMALL_FLAG_DATA) {
        printf("\t- destination_node_id = %u\n", dst_id);
        printf("\t- flag = %s\n", "DATA");
    }
    printf("\t- payload = %u\n", payload);

err:
    axiom_close(dev);


    return 0;
}
