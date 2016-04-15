/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-23
 *
 * This file implementas the axiom-traceroute application
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
#include <signal.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

int verbose = 0;

static void usage(void)
{
    printf("usage: ./axiom-traceroute [[-n dest_node] | [-v] | [-h]] \n");
    printf("AXIOM traceroute\n\n");
    printf("-n,             dest_node   destination node of traceroute\n");
    printf("-v, --verbose               verbose output\n");
    printf("-h, --help                  print this help\n\n");
}

static int
recv_tracereoute_reply(axiom_dev_t *dev, axiom_node_id_t *recv_node,
                       axiom_port_t *port, axiom_flag_t *flag,
                       axiom_traceroute_payload_t *recv_payload)
{
    axiom_msg_id_t msg_err;

    msg_err =  axiom_recv_small(dev, recv_node, port, flag, (axiom_payload_t*)recv_payload);
    if (msg_err == AXIOM_RET_ERROR)
    {
        EPRINTF("receive error");
        return -1;
    }
    if (*flag == AXIOM_SMALL_FLAG_NEIGHBOUR)
    {
        EPRINTF("receive AXIOM_SMALL_FLAG_NEIGHBOUR message");
        return -1;
    }
    if (*port != AXIOM_SMALL_PORT_NETUTILS)
    {
        EPRINTF("port not equal to AXIOM_SMALL_PORT_INIT");
        return -1;
    }
    if (recv_payload->command != AXIOM_CMD_TRACEROUTE_REPLY)
    {
        EPRINTF("message not equal to AXIOM_CMD_TRACEROUTE_REPLY");
        return -1;
    }

    return 0;
}

axiom_err_t
axiom_next_hop(axiom_dev_t *dev, axiom_node_id_t dest_node_id,
               axiom_if_id_t *my_if) {
    axiom_err_t ret;
    int i;

    ret = axiom_get_routing(dev, dest_node_id, my_if);
    for (i = 0; i < 4; i++)
    {
        if (*my_if & (axiom_node_id_t)(1<<i))
        {
            *my_if = i;
            break;
        }
    }
    return ret;
}

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_node_id_t dest_node, recv_node = 0;
    axiom_if_id_t my_if;
    axiom_err_t err;
    axiom_msg_id_t msg_err, my_node_id;
    axiom_port_t port;
    axiom_flag_t flag;
    axiom_traceroute_payload_t payload, recv_payload;
    uint8_t received_reply = 0, expected_reply = 0;
    int dest_node_ok = 0;
    int long_index =0;
    int opt = 0;
    int recv_err;
    static struct option long_options[] = {
        {"n", required_argument, 0, 'n'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv,"vhn:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'n' :
                if (sscanf(optarg, "%" SCNu8, &dest_node) != 1)
                {
                    EPRINTF("wrong number of nodes");
                    usage();
                    exit(-1);
                }
                dest_node_ok = 1;
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

    /* check if dest_node parameter has been inserted */
    if (dest_node_ok == 1)
    {
        /* open the axiom device */
        dev = axiom_open(NULL);
        if (dev == NULL) {
            perror("axiom_open()");
            exit(-1);
        }

        my_node_id = axiom_get_node_id(dev);

        /* bind the current process on port */
#if 0
        if (port_ok == 1) {
            err = axiom_bind(dev, AXIOM_SMALL_PORT_NETUTILS);
        }
#endif

        /* get interface to reach next hop for dest_node */
        err = axiom_next_hop(dev, dest_node, &my_if);
        if (err == AXIOM_RET_ERROR)
        {
            EPRINTF("axiom_next_hop error");
            exit(-1);
        }

        printf("Node %u, start traceroute of node = %u\n\n",
               my_node_id, dest_node);

        flag = AXIOM_SMALL_FLAG_NEIGHBOUR;
        port = AXIOM_SMALL_PORT_NETUTILS;
        payload.command = AXIOM_CMD_TRACEROUTE;
        payload.src_id = my_node_id;
        payload.dst_id = dest_node;
        payload.step = 0;
        /* send small neighbour traceroute message */
        msg_err = axiom_send_small(dev, my_if, port, flag,
                                   (axiom_payload_t *)&payload);

        if (msg_err == AXIOM_RET_ERROR)
        {
            EPRINTF("send error");
            goto err;
        }
        memset (&recv_payload, 0, sizeof(recv_payload));
        do
        {
            recv_err = recv_tracereoute_reply(dev, &recv_node, &port,
                                              &flag, &recv_payload);
            if (recv_err == -1)
            {
                goto err;
            }
            received_reply++;
            printf("[hop %u] \n", recv_payload.step);
            printf("\t- node_id = %u\n\n", recv_node);

            /* wait for traceroute destination node reply*/
        } while (recv_node != dest_node);

        if (received_reply == recv_payload.step)
        {
            /* the number of steps present into the traceroute
             * destination node reply is equal to the number of
             * the already received messages: end of traceroute */
            printf("End traceroute\n");
        }
        else
        {
            /* I have to receive other messages */
            expected_reply = recv_payload.step;
            while (received_reply != expected_reply)
            {
                recv_err = recv_tracereoute_reply(dev, &recv_node, &port,
                                                  &flag, &recv_payload);
                if (recv_err == -1)
                {
                    goto err;
                }
                received_reply++;
                printf("[hop %u] \n", recv_payload.step);
                printf("\t- node_id = %u\n\n", recv_node);
            }
            printf("End traceroute\n");
        }
    }
    else
    {
        usage();
        exit(-1);
    }

err:
    axiom_close(dev);

    return 0;

}
