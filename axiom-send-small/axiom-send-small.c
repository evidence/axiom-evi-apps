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
    int port, dst_id, port_ok = 0, dst_ok = 0, payload_ok = 0;
    axiom_flag_t flag = 0;
    axiom_payload_t payload;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"id", required_argument, 0, 'i'},
        {"port", required_argument, 0, 'p'},
        {"payload", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"i:p:",
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

            case 'i' :
               if (sscanf(optarg, "%i", &dst_id) != 1)
               {
                   printf("please use option --id 'destination_node_id'\n");
                   exit(-1);
               }
               else
               {
                   dst_ok = 1;
               }
               break;

            case 'm' :
               if (sscanf(optarg, "%i", &payload) != 1)
               {
                   printf("please use option --payload 'payload'\n");
                   exit(-1);
               }
               else
               {
                   payload_ok = 1;
               }
               break;

            case '?':
                printf ("Specific arguments are required\n");
                exit(-1);

             default:
                printf("Allowed paramater are --id 'destination_node_id' -port 'port_number'\n");
                exit(-1);
        }
    }

    /* check port parameter */
    if (port_ok == 1)
    {
        /* port arameter inserted */
        if ((port <= 0) || (port > 255))
        {
            printf("Allowed port number = %i; [0 < port < 256]\n", port);
            exit(-1);
        }
        else
        {
            printf("port number = %i\n", port);
        }
    }
    else
    {
        printf("please use all options\n");
        exit(-1);
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
        printf("please use all options\n");
        exit(-1);
    }

    /* check payload parameter */
    if (payload_ok == 1)
    {
        printf("payload = %i\n", payload);
    }
    else
    {
        printf("please use all options\n");
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
        printf("\t- destination_node_id = %d\n", dst_id);
        printf("\t- flag = %d\n", flag);
        printf("\t- payload = %d\n", payload);
    }

    axiom_close(dev);


    return 0;
}
