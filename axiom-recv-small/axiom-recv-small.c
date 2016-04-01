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

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t recv_ret;
    axiom_node_id_t src_id;
    int port, port_ok = 0;
    axiom_flag_t flag;
    axiom_payload_t payload;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
            {"port", required_argument, 0, 'p'},
            {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"p:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
             case 'p' :
                if (sscanf(optarg, "%i", &port) != 1)
                {
                    printf("please use option --port 'port_number'\n");
                    exit(-1);
                }
                else
                {
                    port_ok = 1;
                }
                break;

            case '?':
                printf ("An argument is required\n");
                exit(-1);

             default:
                printf("Allowed paramater is --port 'port_number'\n");
                exit(-1);
        }
    }

    if (port_ok == 1)
    {
        /* port arameter inserted */
        if ((port <= 0) || (port > 255))
        {
            printf("port number = %i; [0 < port < 256]\n", port);
            exit(-1);
        }
        else
        {
            printf("port number = %i\n", port);
        }
    }
    else
    {
        printf("please use option --port 'port_number'\n");
        exit(-1);
    }

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    /* bind the current process on port */
    /* err = axiom_bind(dev, port); */

    /* receive a small message from port*/
    recv_ret =  axiom_recv_small(dev, &src_id, (axiom_port_t *)&port, &flag, &payload);
    if (recv_ret == AXIOM_RET_ERROR)
    {
           printf("Receive error");
    }
    else
    {
        printf("Message received on port %d\n", port);
        printf("\t- source_node_id = %d\n", src_id);
        printf("\t- flag = %d\n", flag);
        printf("\t- payload = %d\n", payload);
    }

    axiom_close(dev);


    return 0;
}
