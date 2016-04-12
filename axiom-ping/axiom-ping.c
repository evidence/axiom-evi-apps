/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
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
#include <inttypes.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "dprintf.h"

static void usage(void)
{
    printf("usage: ./axiom-ping [[-p port] | [-h]] -d dest \n");
    printf("AXIOM ping\n\n");
    printf("-p, --port      port     port used for sending\n");
    printf("-d, --dest      dest     dest node id \n");
    printf("-h, --help               print this help\n\n");
}

/*  This function subtracts the `struct timeval' values x and y,
    storing the result in diff.
    Return 1 if the difference is negative, otherwise 0.  */
 static int timeval_subtract (struct timeval *diff,
                              struct timeval *x, struct timeval *y)
 {
    int nsec;

    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec)
    {
        nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000)
    {
        nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute difference */
    diff->tv_sec = x->tv_sec - y->tv_sec;
    diff->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return (x->tv_sec < y->tv_sec);
 }


int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t send_ret, recv_ret, my_node_id;
    axiom_port_t port = 1, recv_port ;
    axiom_node_id_t dst_id, src_id;
    int port_ok = 0, dst_ok = 0;
    axiom_flag_t flag = 0;
    axiom_payload_t payload = 0, recv_payload;
    struct timeval start_tv, end_tv, diff_tv;
    int ret;


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


    while ((opt = getopt_long(argc, argv,"hp:d:",
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

            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    /* check port parameter */
    if (port_ok == 1)
    {
        /* port parameter inserted */
        if ((port <= 0) || (port > 255))
        {
            printf("Port not allowed [%u]; [0 < port < 256]\n", port);
            exit(-1);
        }
    }

    /* check destination node parameter */
    if (dst_ok == 1)
    {
        /* destination node inserted */
        if ((dst_id < 0) || (dst_id > 254))
        {
            printf("Destination node id not allowed [%u]; [0 <= dst_id < 255]\n", dst_id);
            exit(-1);
        }
    }
    else
    {
        usage();
        exit(-1);
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

    do
    {
        /* get actual time */
        ret = gettimeofday(&start_tv,NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        printf("\n\nNew Message\n");
        printf("timestamp: %ld sec\t%ld microsec\n", start_tv.tv_sec,
                                                   start_tv.tv_usec);
        printf("[node %u] sending ping message...\n", my_node_id);

        /* send a small message*/
        payload = (axiom_payload_t)(payload + 1);
        send_ret =  axiom_send_small(dev, (axiom_node_id_t)dst_id,
                                            (axiom_port_t)port, flag,
                                            (axiom_payload_t*)&payload);
        if (send_ret == AXIOM_RET_ERROR)
        {
               EPRINTF("receive error");
               goto err;
        }
        printf("[node %u] message sent to port %u\n", my_node_id, port);
        printf("\t- destination_node_id = %u\n", dst_id);

        /* ********************* receive the reply ************************ */
        /* receive a small message from port*/
        recv_ret =  axiom_recv_small(dev, &src_id, (axiom_port_t *)&recv_port,
                                     &flag, &recv_payload);
        if (recv_ret == AXIOM_RET_ERROR)
        {
            EPRINTF("receive error");
            break;
        }

        /* get actual time */
        ret = gettimeofday(&end_tv,NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        printf("[node %u] reply received on port %u\n", my_node_id, recv_port);
        printf("\t- source_node_id = %u\n", src_id);
        printf("\t- message index = %u\n", recv_payload);
        printf("timestamp: %ld sec\t%ld microsec\n", end_tv.tv_sec,
                                                   end_tv.tv_usec);

        ret = timeval_subtract (&diff_tv, &end_tv, &start_tv);
        if (ret == 1)
        {
            EPRINTF("negative rtt!");
            goto err;
        }
        printf("RTT: %ld sec\t%ld microsec\n", diff_tv.tv_sec,
                                             diff_tv.tv_usec);
    } while (1);

err:
    axiom_close(dev);


    return 0;
}
