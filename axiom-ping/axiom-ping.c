/*!
 * \file axiom-ping.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-ping application.
 *
 * axiom-ping estimate the round trip time (RTT) between two axiom nodes.
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
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_packets.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

//#define AXIOM_NO_TX

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-ping [arguments] -d dest_node \n");
    printf("AXIOM ping: estimate the round trip time (RTT) between this node\n");
    printf("            and the specified dest_node\n");
    printf("Arguments:\n");
    printf("-d, --dest       dest_node   destination node id of axiom-ping\n");
    printf("-i, --interval   interval    sec between two ping messagges \n");
    printf("-c, --count      count       number of ping messagges to send \n");
    printf("-v, --verbose                verbose output\n");
    printf("-h, --help                   print this help\n\n");
}

static volatile sig_atomic_t sigint_received = 0;

/* control-C handler */
static void
sigint_handler(int sig)
{
    sigint_received = 1;
}


int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t send_ret, recv_ret, node_id;
    axiom_port_t remote_port = AXIOM_SMALL_PORT_INIT, recv_port;
    axiom_port_t port = AXIOM_SMALL_PORT_NETUTILS;
    axiom_node_id_t dst_id, src_id;
    axiom_ping_payload_t payload, recv_payload;
    axiom_err_t err;
    struct sigaction sig;
    double interval = 1; /* default interval 1 sec */
    unsigned int num_ping = 1;
    int dst_ok = 0, num_ping_ok = 0;
    uint64_t diff_nsec, sum_nsec = 0, max_nsec = 0, min_nsec = -1;
    double avg_nsec, packet_loss;
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

    /* set hanlder for signal SIGINT */
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = sigint_handler;
    sigaction(SIGINT, &sig, NULL);

    while ((opt = getopt_long(argc, argv,"vhd:i:c:",
                         long_options, &long_index )) != -1) {
        switch (opt) {
            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &dst_id) != 1) {
                    EPRINTF("wrong destination");
                    usage();
                    exit(-1);
                }
                dst_ok = 1;
                break;

            case 'i':
                if (sscanf(optarg, "%lf", &interval) != 1) {
                    EPRINTF("wrong interval");
                    usage();
                    exit(-1);
                }
                break;

            case 'c':
                if (sscanf(optarg, "%" SCNu32, &num_ping) != 1) {
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
    if (dst_ok == 1) {
        /* destination node inserted */
        if ((dst_id < 0) || (dst_id > 254)) {
            printf("Destination node id not allowed [%u]; [0 <= dst_id < 255]\n",
                    dst_id);
            exit(-1);
        }
    } else {
        usage();
        exit(-1);
    }
#ifndef AXIOM_NO_TX
    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    /* bind the current process on local port */
    err = axiom_bind(dev, port);
    if (err == AXIOM_RET_ERROR) {
        EPRINTF("bind error");
        goto err;
    }
#endif
    printf("PING node %d.\n", dst_id);

    payload.packet_id = 0;
    while (!sigint_received &&  (num_ping > 0)) {
        struct timespec start_ts, end_ts, diff_ts;
        axiom_type_t type = AXIOM_SMALL_TYPE_DATA;
        int retry;

        IPRINTF(verbose,"[node %u] sending ping message...\n", node_id);
#ifndef AXIOM_NO_TX
        /* send a small message*/
        payload.command = AXIOM_CMD_PING;
        payload.packet_id = payload.packet_id + 1;
        send_ret =  axiom_send_small(dev, (axiom_node_id_t)dst_id,
                remote_port, type, (axiom_payload_t*)&payload);
        if (send_ret == AXIOM_RET_ERROR) {
            EPRINTF("send error");
            goto err;
        }
#endif
        /* get actual time */
        ret = clock_gettime(CLOCK_REALTIME, &start_ts);
        if (ret == -1) {
            EPRINTF("gettime error");
            goto err;
        }
        sent_packets++;

        IPRINTF(verbose,"[node %u] message sent to port %u\n", node_id,
                remote_port);
        IPRINTF(verbose,"\t- destination_node_id = %u\n", dst_id);

        /* ********************* receive the reply ************************ */
        retry = 1;
        do {
#ifndef AXIOM_NO_TX
            /* receive a ping reply (pong) */
            recv_ret =  axiom_recv_small(dev, &src_id, (axiom_port_t *)&recv_port,
                    &type, (axiom_payload_t*)&recv_payload);
#else
            usleep(1);
#endif
            /* get actual time */
            ret = clock_gettime(CLOCK_REALTIME, &end_ts);
            if (ret == -1) {
                EPRINTF("gettime error");
                goto err;
            }
#ifdef AXIOM_NO_TX
            break;
#endif
            if (sigint_received) {
                sent_packets--;
                goto exit;
            }

            if (recv_ret == AXIOM_RET_ERROR) {
                EPRINTF("receive error");
                goto err;
            }

            if (recv_payload.command != AXIOM_CMD_PONG ||
                    recv_payload.packet_id != payload.packet_id) {
                IPRINTF(verbose, "receive a no AXIOM-PONG message");
                retry = 1;
            } else {
                retry = 0;
            }

        } while (retry);

        recv_packets++;
        IPRINTF(verbose,"[node %u] reply received on port %u\n", node_id,
                recv_port);
        IPRINTF(verbose,"\t- source_node_id = %u\n", src_id);
        IPRINTF(verbose,"\t- message index = %u\n", recv_payload.packet_id);

        diff_ts = timespec_sub(end_ts, start_ts);

        /* ********************** statiscs ***************************** */
        /* difference computation */
        diff_nsec = timespec2nsec(diff_ts);

        sum_nsec += diff_nsec;

        printf("from node %d: seq_num=%d time=%3.3f ms\n", src_id,
                recv_payload.packet_id,
                nsec2msec(diff_nsec));

        if (diff_nsec > max_nsec) {
            max_nsec = diff_nsec;
        }
        if (diff_nsec < min_nsec) {
            min_nsec = diff_nsec;
        }

        if (sigint_received) {
            break;
        }

        if (interval)
            usleep(interval * 1000000);

        if (num_ping_ok == 1) {
            num_ping--;
        }

    }

exit:
    /* average computation */
    avg_nsec = ((double)(sum_nsec) / recv_packets);
    packet_loss = (1 - ((double)(recv_packets) / sent_packets)) * 100;

    printf("\n--- node %d ping statistics ---\n", dst_id);
    printf("%d packets transmitted, %d received, %2.2f%% packet loss\n",
           sent_packets, recv_packets, packet_loss);
    if (recv_packets) {
        printf("rtt min/avg/max = %3.3f/%3.3f/%3.3f ms\n", nsec2msec(min_nsec),
                nsec2msec(avg_nsec), nsec2msec(max_nsec));
    }

err:
    axiom_close(dev);


    return 0;
}
