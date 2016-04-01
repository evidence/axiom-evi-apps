/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.2
 * Last update: 2016-03-23
 *
 * This file tests the AXIOM INIT implementation
 *
 */

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
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"

static void usage(void)
{
    printf("usage: ./axiom-sen-small [[-p port] [-n] [-h]] -d dest -l payload\n\n");
    printf("-p, --port      port     port used for sending\n");
    printf("-d, --dest      dest     dest node id or local if (TO_NEIGHBOUR) \n");
    printf("-n, --neighbour          send to neighbour\n");
    printf("-l, --payload   payload  message to send \n");
    printf("-h, --help               print this help\n");
}

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t recv_ret, my_node_id;
    int port = 1, dst_id, port_ok = 0, dst_ok = 0, payload_ok = 0, to_neighbour = 0;
    axiom_flag_t flag = 0;
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
                if (sscanf(optarg, "%i", &port) != 1)
                {
                    usage();
                    exit(-1);
                }
                else
                {
                    port_ok = 1;
                }
                break;

            case 'd' :
               if (sscanf(optarg, "%i", &dst_id) != 1)
               {
                   usage();
                   exit(-1);
               }
               else
               {
                   dst_ok = 1;
               }
               break;

            case 'n' :
               to_neighbour = 1;
               break;

            case 'l' :
               if (sscanf(optarg, "%i", &payload) != 1)
               {
                   usage();
                   exit(-1);
               }
               else
               {
                   payload_ok = 1;
               }
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
        if ((port <= 0) || (port > 255))
        {
            printf("Port not allowed [%i]; [0 < port < 256]\n", port);
            exit(-1);
        }
        else
        {
            printf("port number = %i\n", port);
        }
    }

    /* check destination node parameter */
    if (dst_ok == 1)
    {
        /* port arameter inserted */
        if ((dst_id < 0) || (dst_id > 255))
        {
            printf("Allowed destination node id = %i; [0 <= dst_id < 256]\n", dst_id);
            exit(-1);
        }
        else
        {
            printf("destination node id = %i\n", dst_id);
        }
    }
    else
    {
        usage();
        exit(-1);
    }

    /* check payload parameter */
    if (payload_ok == 1)
    {
        printf("payload = %i\n", payload);
    }
    else
    {
        usage();
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

    printf("[node %d] sending small message...\n", my_node_id);
    /* send a small message*/
    recv_ret =  axiom_send_small(dev, (axiom_node_id_t)dst_id,
                                (axiom_port_t)port, flag,
                                 (axiom_payload_t*)&payload);
    if (recv_ret == AXIOM_RET_ERROR)
    {
           printf("Receive error");
    }
    else
    {
        printf("Message sent to port %d\n", port);
        if (flag & AXIOM_SMALL_FLAG_NEIGHBOUR) {
            printf("\t- local_interface = %d\n", dst_id);
        } else {
            printf("\t- destination_node_id = %d\n", dst_id);
        }
        printf("\t- flag = %d\n", flag);
        printf("\t- payload = %d\n", payload);
    }

    axiom_close(dev);


    return 0;
}
