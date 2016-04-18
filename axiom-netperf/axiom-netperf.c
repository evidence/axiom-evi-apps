/*
 * axiom_netperf.c
 *
 * Version:     v0.3.1
 * Last update: 2016-04-15
 *
 * This file implementas the axiom-netperf application
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

#define AXIOM_NETPERF_BYTES 1024

int verbose = 0;

static double usec2msec(uint64_t usec)
{
    return ((double)(usec) / 1000);
}

/* receive elapsed time from remote node */
static int recv_elapsed_time(axiom_dev_t *dev,  axiom_node_id_t *recv_node,
                             axiom_port_t *port, axiom_flag_t *flag,
                             axiom_payload_t *recv_payload,
                             uint64_t *elapsed_rx_usec) {
    axiom_msg_id_t msg_err;
    int i;

    for (i = 0; i < sizeof(uint64_t)/2; i++)
    {
        msg_err =  axiom_recv_small(dev, recv_node, port, flag, recv_payload);
        if (msg_err == AXIOM_RET_ERROR)
        {
            return -1;
        }
        if (((axiom_netperf_time_payload_t*)recv_payload)->command != AXIOM_CMD_NETPERF)
        {
            return -1;
        }
        *elapsed_rx_usec |=  (uint64_t)(((axiom_netperf_time_payload_t*)recv_payload)->time <<
                                        (((axiom_netperf_time_payload_t*)recv_payload)->byte_order*16));
    }

    return 0;
}

static void usage(void)
{
    printf("usage: ./axiom-netperf [[-n dest_node] | [-v] | [-h]] \n");
    printf("AXIOM traceroute\n\n");
    printf("-n,             dest_node   destination node of netperf\n");
    printf("-v, --verbose               verbose output\n");
    printf("-h, --help                  print this help\n\n");
}

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t msg_err;
    axiom_node_id_t dest_node, recv_node;
    int dest_node_ok = 0, i, err, ret;
    axiom_port_t port;
    axiom_flag_t flag;
    axiom_netperf_payload_t payload;
    axiom_netperf_time_payload_t recv_payload;
    struct timeval start_tv, end_tv, elapsed_tv;
    uint64_t elapsed_usec, elapsed_rx_usec;
    double tx_th, rx_th;
    int long_index = 0;
    int opt = 0;
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

        /* bind the current process on port */
#if 0
        err = axiom_bind(dev, AXIOM_SMALL_PORT_NETUTILS);
        if (err == AXIOM_RET_ERROR)
        {
            EPRINTF("axiom_bind error");
            goto err;
        }
#endif
        /* get time of the first sent netperf message */
        ret = gettimeofday(&start_tv, NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        IPRINTF(verbose,"Start timestamp: %ld sec\t%ld microsec\n", start_tv.tv_sec,
                                                              start_tv.tv_usec);

        for (i = 0; i < (AXIOM_NETPERF_BYTES / sizeof(axiom_small_msg_t)); i++)
        {
            payload.command = AXIOM_CMD_NETPERF;
            payload.total_bytes = AXIOM_NETPERF_BYTES;
            /* send netperf message */
            msg_err = axiom_send_small(dev, dest_node, AXIOM_SMALL_PORT_INIT, 0,
                                       (axiom_payload_t *)&payload);
            if (msg_err == AXIOM_RET_ERROR)
            {
                EPRINTF("send error");
                goto err;
            }
        }

        /* get time of the last sent netperf message */
        ret = gettimeofday(&end_tv, NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        IPRINTF(verbose,"End timestamp: %ld sec\t%ld microsec\n", end_tv.tv_sec,
                                                              end_tv.tv_usec);

        /* compute time elapsed ms */
        timersub(&end_tv, &start_tv, &elapsed_tv);
        elapsed_usec = elapsed_tv.tv_usec + (elapsed_tv.tv_sec * 1000000);

        /* receive elapsed rx throughput time form dest_node */
        elapsed_rx_usec = 0;
        err = recv_elapsed_time(dev, &recv_node, &port, &flag,
                          (axiom_payload_t*)&recv_payload, &elapsed_rx_usec);
        if ((err == -1) || (recv_node != dest_node))
        {
            EPRINTF("recv_elapsed_time error");
            goto err;
        }

        tx_th = (double)(AXIOM_NETPERF_BYTES / usec2msec(elapsed_usec));
        rx_th = (double)(AXIOM_NETPERF_BYTES / usec2msec(elapsed_rx_usec));

        printf("Tx throughput = =%3.3f bytes/ms\n", tx_th);
        printf("Rx throughput = =%3.3f bytes/ms\n", rx_th);

    }

err:
    axiom_close(dev);

    return 0;
}
