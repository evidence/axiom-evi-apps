/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-23
 *
 * This file implementas the AXIOM PING application
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

#include "axiom_nic_packets.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

//#define AXIOM_NO_TX

int verbose = 0;

static void usage(void)
{
    printf("usage: axiom-ping [arguments] -d dest_node \n");
    printf("AXIOM ping: estimate the round trip time (RTT) between this node\n");
    printf("            and the specified dest_node\n");
    printf("Arguments:\n");
    printf("-d, --dest       dest_node   destination node id of axiom-ping\n");
    printf("-i, --interval   interval    ms between two ping messagges \n");
    printf("-c, --count      count       number of ping messagges to send \n");
    printf("-v, --verbose                verbose output\n");
    printf("-h, --help                   print this help\n\n");
}

static volatile sig_atomic_t sigint_received = 0;

/* control-C handler */
static void sigint_handler(int sig)
{
    sigint_received = 1;
}

/* TODO: allow the user to change the resolution */
static double usec2msec(uint64_t usec)
{
    return ((double)(usec) / 1000);
}


int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t send_ret, recv_ret, my_node_id;
    axiom_port_t remote_port = AXIOM_SMALL_PORT_INIT, recv_port;
    axiom_port_t my_port = AXIOM_SMALL_PORT_NETUTILS;
    axiom_node_id_t dst_id, src_id;
    axiom_ping_payload_t payload, recv_payload;
    axiom_err_t err;
    struct sigaction sig;
    unsigned int interval = 500; /* default interval 500 ms */
    unsigned int num_ping = 1;
    int dst_ok = 0, num_ping_ok = 0;
    struct timeval start_tv, end_tv, diff_tv;
    uint64_t diff_usec, sum_usec = 0, max_usec = 0, min_usec = -1;
    double avg_usec, packet_loss;
    int sent_packets = 0, recv_packets = 0;
    int ret;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"interval", required_argument, 0, 'i'},
        {"count", required_argument, 0, 'c'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    /* set my_hanlder for signal SIGINT */
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = sigint_handler;
    sigaction(SIGINT, &sig, NULL);

    while ((opt = getopt_long(argc, argv,"vhd:i:c:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &dst_id) != 1)
                {
                    EPRINTF("wrong destination");
                    usage();
                    exit(-1);
                }
                dst_ok = 1;
                break;

            case 'i':
                if (sscanf(optarg, "%" SCNu32, &interval) != 1)
                {
                    EPRINTF("wrong interval");
                    usage();
                    exit(-1);
                }
                break;

            case 'c':
                if (sscanf(optarg, "%" SCNu32, &num_ping) != 1)
                {
                    EPRINTF("wrong count");
                    usage();
                    exit(-1);
                }
                num_ping_ok = 1;
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
#ifndef AXIOM_NO_TX
    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    my_node_id = axiom_get_node_id(dev);
#endif
    /* bind the current process on my port */
    err = axiom_bind(dev, my_port);
    if (err == AXIOM_RET_ERROR)
    {
        EPRINTF("send error");
        goto err;
    }



    printf("PING node %d.\n", dst_id);

    payload.packet_id = 0;
    while (!sigint_received &&  (num_ping > 0))
    {
        axiom_flag_t flag = AXIOM_SMALL_FLAG_DATA;

        IPRINTF(verbose,"[node %u] sending ping message...\n", my_node_id);
#ifndef AXIOM_NO_TX
        /* send a small message*/
        payload.command = AXIOM_CMD_PING;
        payload.packet_id = (uint16_t)(payload.packet_id + 1);
        send_ret =  axiom_send_small(dev, (axiom_node_id_t)dst_id,
                                            remote_port, flag,
                                            (axiom_payload_t*)&payload);
        if (sigint_received) {
            break;
        }
        if (send_ret == AXIOM_RET_ERROR)
        {
               EPRINTF("send error");
               goto err;
        }
#endif
        /* get actual time */
        ret = gettimeofday(&start_tv,NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        IPRINTF(verbose,"timestamp: %ld sec\t%ld microsec\n", start_tv.tv_sec,
                                                              start_tv.tv_usec);
        sent_packets++;
#ifndef AXIOM_NO_TX
        IPRINTF(verbose,"[node %u] message sent to port %u\n", my_node_id, remote_port);
        IPRINTF(verbose,"\t- destination_node_id = %u\n", dst_id);

        /* ********************* receive the reply ************************ */
        /* receive a small message from port*/
        recv_ret =  axiom_recv_small(dev, &src_id, (axiom_port_t *)&recv_port,
                                     &flag, (axiom_payload_t*)&recv_payload);
        /* get actual time */
        ret = gettimeofday(&end_tv,NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }

        if (sigint_received) {
            break;
        }
        if (recv_ret == AXIOM_RET_ERROR)
        {
            EPRINTF("receive error");
            break;
        }
#else
        usleep(1234000);
#endif

        if (recv_payload.command != AXIOM_CMD_PONG)
        {
            EPRINTF("receive a no AXIOM-PONG message");
            continue;
        }

        recv_packets++;
        IPRINTF(verbose,"[node %u] reply received on port %u\n", my_node_id, recv_port);
        IPRINTF(verbose,"\t- source_node_id = %u\n", src_id);
        IPRINTF(verbose,"\t- message index = %u\n", recv_payload.packet_id);
        IPRINTF(verbose,"timestamp: %ld sec\t%ld microsec\n", end_tv.tv_sec,
                                                   end_tv.tv_usec);
        /* TODO: check serial number? */

        timersub(&end_tv, &start_tv, &diff_tv);

        /* ********************** statiscs ***************************** */
        /* difference computation */
        diff_usec = diff_tv.tv_usec + (diff_tv.tv_sec * 1000000);
        sum_usec += diff_usec;

        printf("from node %d: seq_num=%d time=%3.3f ms\n", src_id,
                recv_payload.packet_id,
                usec2msec(diff_usec));

        if (diff_usec > max_usec)
        {
            max_usec = diff_usec;
        }
        if (diff_usec < min_usec)
        {
            min_usec = diff_usec;
        }

        usleep(interval * 1000);

        if (num_ping_ok == 1)
        {
            num_ping--;
        }

    }

    /* average computation */
    avg_usec = ((double)(sum_usec) / recv_packets);
    packet_loss = (1 - ((double)(recv_packets) / sent_packets)) * 100;

    printf("\n--- node %d ping statistics ---\n", dst_id);
    printf("%d packets transmitted, %d received, %2.2f%% packet loss\n",
           sent_packets, recv_packets, packet_loss);
    if (recv_packets) {
        printf("rtt min/avg/max = %3.3f/%3.3f/%3.3f ms\n", usec2msec(min_usec),
                usec2msec(avg_usec), usec2msec(max_usec));
    }

err:
    axiom_close(dev);


    return 0;
}
