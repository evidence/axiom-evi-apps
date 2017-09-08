/*!
 * \file axiom-ping.c
 *
 * \version     v0.14
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-ping application.
 *
 * axiom-ping estimate the round trip time (RTT) between two axiom nodes.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
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
#include <pthread.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_packets.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

struct sender_param {
    axiom_dev_t *dev;
    axiom_node_id_t dst_id;
    axiom_node_id_t node_id;
    int num_ping_set;
    int num_ping;
    uint32_t unique_id;
    double interval;
};

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
static int sent_packets = 0, recv_packets = 0;

/* control-C handler */
static void
sigint_handler(int sig)
{
    sigint_received = 1;
}

void *
sender_body(void *opaque)
{
    struct sender_param *p = (struct sender_param *)opaque;
    axiom_port_t remote_port = AXIOM_RAW_PORT_INIT;
    axiom_ping_payload_t payload;

    payload.command = AXIOM_CMD_PING;
    payload.unique_id = p->unique_id;
    payload.seq_num = 0;

    while (!sigint_received && (p->num_ping > 0)) {
        axiom_type_t type = AXIOM_TYPE_RAW_DATA;
        axiom_err_t send_ret;
        struct timespec start_ts;
        int ret;

        IPRINTF(verbose,"[node %u] sending ping message...\n", p->node_id);

        /* get actual time */
        ret = clock_gettime(CLOCK_REALTIME, &start_ts);
        if (ret == -1) {
            EPRINTF("gettime error");
            goto err;
        }

        /* send a raw ping  message*/
        payload.seq_num = payload.seq_num + 1;
        payload.timestamp = timespec2nsec(start_ts);
        send_ret =  axiom_send_raw(p->dev, (axiom_node_id_t)p->dst_id,
                remote_port, type, sizeof(payload), &payload);
        if (!AXIOM_RET_IS_OK(send_ret)) {
            IPRINTF(verbose, "send error");
            goto err;
        }

        sent_packets++;

        IPRINTF(verbose,"[node %u] message sent to port %u\n", p->node_id,
                remote_port);
        IPRINTF(verbose,"\t- destination_node_id = %u\n", p->dst_id);


        if (p->interval && !sigint_received)
            usleep(p->interval * 1000000);

        if (p->num_ping_set == 1) {
            p->num_ping--;
        }
    }

err:

    return NULL;
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_port_t recv_port;
    axiom_port_t port = AXIOM_RAW_PORT_NETUTILS;
    axiom_node_id_t dst_id, src_id, node_id;
    axiom_ping_payload_t recv_payload;
    axiom_err_t err, recv_ret;
    struct sigaction sig;
    double interval = 1; /* default interval 1 sec */
    unsigned int num_ping = 1;
    int dst_ok = 0, num_ping_set = 0;
    uint32_t unique_id;
    uint64_t sum_nsec = 0, max_nsec = 0, min_nsec = -1;
    double avg_nsec, packet_loss;
    int ret;
    struct sender_param sender_param;
    pthread_t sender_thread;

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
                num_ping_set = 1;
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

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    /* bind the current process on local port */
    err = axiom_bind(dev, port);
    if (err != port) {
        EPRINTF("bind error");
        goto err;
    }

    /* generate random packet id */
    srand(time(NULL));
    unique_id = rand();

    printf("PING node %d.\n", dst_id);

    sender_param.dev = dev;
    sender_param.node_id = node_id;
    sender_param.dst_id = dst_id;
    sender_param.unique_id = unique_id;
    sender_param.interval = interval;
    sender_param.num_ping = num_ping;
    sender_param.num_ping_set = num_ping_set;

    /* start sender thread */
    if (pthread_create(&sender_thread, NULL, sender_body, &sender_param)) {
        EPRINTF("error creating sender thread");
        goto err;
    }

    while (!sigint_received &&  (num_ping > 0)) {
        struct timespec end_ts;
        axiom_type_t type = AXIOM_TYPE_RAW_DATA;
        uint64_t diff_ns, start_ns, end_ns;
        int retry;

        IPRINTF(verbose,"[node %u] receiving ping message...\n", node_id);


        /* ********************* receive the reply ************************ */
        retry = 1;
        do {
            axiom_raw_payload_size_t payload_size = sizeof(recv_payload);

            /* receive a ping reply (pong) */
            recv_ret =  axiom_recv_raw(dev, &src_id, (axiom_port_t *)&recv_port,
                    &type, &payload_size, &recv_payload);

            /* get actual time */
            ret = clock_gettime(CLOCK_REALTIME, &end_ts);
            if (ret == -1) {
                EPRINTF("gettime error");
                goto err;
            }

            if (!AXIOM_RET_IS_OK(recv_ret)) {
                /* recv interrupted by the SIGINT signal */
                if (recv_ret == AXIOM_RET_INTR) {
                    goto exit;
                }

                EPRINTF("receive error");
                goto err;
            }

            if (recv_payload.command != AXIOM_CMD_PONG ||
                    recv_payload.unique_id != unique_id) {
                IPRINTF(verbose, "received a no AXIOM-PONG message - "
                        "command: 0x%x [0x%x] unique_id 0x%x [0x%x]",
                        recv_payload.command, AXIOM_CMD_PONG,
                        recv_payload.unique_id, unique_id);
                retry = 1;
            } else {
                retry = 0;
            }

        } while (!sigint_received && retry);

        if (sigint_received) {
            break;
        }

        recv_packets++;
        IPRINTF(verbose,"[node %u] reply received on port %u\n", node_id,
                recv_port);
        IPRINTF(verbose,"\t- source_node_id = %u\n", src_id);
        IPRINTF(verbose,"\t- message index = %u\n", recv_payload.unique_id);

        start_ns = recv_payload.timestamp;
        end_ns = timespec2nsec(end_ts);

        /* ********************** statiscs ***************************** */
        /* difference computation */
        diff_ns = end_ns - start_ns;

        sum_nsec += diff_ns;

        printf("from node %d: seq_num=%d time=%3.3f ms\n", src_id,
                recv_payload.seq_num, nsec2msec(diff_ns));

        if (diff_ns > max_nsec) {
            max_nsec = diff_ns;
        }
        if (diff_ns < min_nsec) {
            min_nsec = diff_ns;
        }

        if (sigint_received) {
            break;
        }

        if (num_ping_set == 1) {
            num_ping--;
        }

    }

exit:
    /* join with the sender thread */
    if (pthread_join(sender_thread, NULL)) {
        EPRINTF("error joining sender thread");
        goto err;
    }

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
