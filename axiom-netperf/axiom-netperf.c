/*!
 * \file axiom-netperf.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-netperf application.
 *
 * axiom-netperf estimates the throughput between two axiom nodes.
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

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

#define AXIOM_NETPERF_DEF_DATA_SCALE    10
#define AXIOM_NETPERF_DEF_CHAR_SCALE    'B'
#define AXIOM_NETPERF_DEF_DATA_LENGTH   512
#define AXIOM_NETPERF_DEF_PAYLOAD_SIZE  128

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-netperf [arguments] -d dest_node \n");
    printf("AXIOM netperf: estimate the throughput between this node and the\n");
    printf("               specified dest_node\n\n");
    printf("Arguments:\n");
    printf("-d, --dest      dest_node   destination node id of axiom-netperf\n");
    printf("-l, --length    x[B|K|M|G]  bytes to send to the destination node\n");
    printf("                            The suffix specifies the length unit\n");
    printf("-p, --payload   size        payload size [1:128] in bytes (def: %d)\n",
            AXIOM_NETPERF_DEF_PAYLOAD_SIZE);
    printf("-v, --verbose               verbose output\n");
    printf("-h, --help                  print this help\n\n");
}

static uint8_t
get_scale(char char_scale) {
    uint8_t scale;

    switch (char_scale) {
        case 'b': case 'B':
            scale = 0;
            break;

        case 'k': case 'K':
            scale = 10;
            break;

        case 'm': case 'M':
            scale = 20;
            break;

        case 'G': case 'g':
            scale = 30;
            break;

        default:
            scale = 0;
    }

    return scale;
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t msg_err;
    axiom_node_id_t dest_node, src_node;
    axiom_port_t port;
    axiom_type_t type;
    axiom_netperf_payload_t payload;
    axiom_payload_size_t pld_recv_size;
    axiom_payload_size_t payload_size = AXIOM_NETPERF_DEF_PAYLOAD_SIZE;
    struct timespec start_ts, end_ts, elapsed_ts;
    double tx_th, rx_th, tx_raw_th, rx_raw_th, tx_pps, rx_pps;
    int dest_node_ok = 0, err, ret, long_index, opt;
    uint64_t elapsed_nsec, elapsed_rx_nsec, sent_bytes, sent_raw_bytes;
    uint64_t total_packets = 0, total_bytes;
    uint16_t data_length = AXIOM_NETPERF_DEF_DATA_LENGTH;
    uint8_t data_scale = AXIOM_NETPERF_DEF_DATA_SCALE;
    char char_scale = AXIOM_NETPERF_DEF_CHAR_SCALE;

    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"length", required_argument, 0, 'l'},
        {"payload", required_argument, 0, 'p'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"vhd:l:p:",
                         long_options, &long_index )) != -1) {
        switch (opt) {
            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &dest_node) != 1) {
                    EPRINTF("wrong number of destination nodes");
                    usage();
                    exit(-1);
                }
                dest_node_ok = 1;
                break;

            case 'l' :
                if (sscanf(optarg, "%" SCNu16 "%c", &data_length, &char_scale) == 0) {
                    EPRINTF("wrong number of length");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                break;

            case 'p' :
                if (sscanf(optarg, "%" SCNu8, &payload_size) != 1) {
                    EPRINTF("wrong number of payload size");
                    usage();
                    exit(-1);
                }
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
    if (dest_node_ok != 1) {
        usage();
        return 0;
    }

    if (payload_size < 1 || payload_size > 128) {
        EPRINTF("payload size must be between 1 and 128 bytes");
        return 0;
    }

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    /* bind the current process on port */
    err = axiom_bind(dev, AXIOM_RAW_PORT_NETUTILS);
    if (err != AXIOM_RET_OK) {
        EPRINTF("axiom_bind error");
        goto err;
    }

    total_bytes = data_length << data_scale;

    /* send start message */
    payload.command = AXIOM_CMD_NETPERF_START;
    payload.total_bytes = total_bytes;
    msg_err = axiom_send_raw(dev, dest_node, AXIOM_RAW_PORT_INIT,
            AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
    if (msg_err != AXIOM_RET_OK) {
        EPRINTF("send error");
        goto err;
    }
    printf("Starting axiom-netperf to node %u\n", dest_node);
    printf("   message type: RAW\n");
    printf("   payload size: %u bytes\n", payload_size);
    printf("   total bytes: %" PRIu64 " bytes\n", total_bytes);

    /* get time of the first sent netperf message */
    ret = clock_gettime(CLOCK_REALTIME, &start_ts);
    if (ret == -1) {
        EPRINTF("gettime error");
        goto err;
    }
    IPRINTF(verbose,"Start timestamp: %ld sec %ld nanosec\n",
            start_ts.tv_sec, start_ts.tv_nsec);

    payload.command = AXIOM_CMD_NETPERF;
    for (sent_bytes = 0; sent_bytes < total_bytes;
            sent_bytes += payload_size) {
        /* send netperf message */
        msg_err = axiom_send_raw(dev, dest_node, AXIOM_RAW_PORT_INIT,
                AXIOM_TYPE_RAW_DATA, payload_size, &payload);
        if (msg_err != AXIOM_RET_OK) {
            EPRINTF("send error");
            goto err;
        }
        total_packets++;
        DPRINTF("NETPERF msg sent to: %u - total_bytes: %" PRIu64
                " sent_bytes: %" PRIu64, dest_node, total_bytes,
                sent_bytes + sizeof(axiom_raw_msg_t));
    }

    /* get time of the last sent netperf message */
    ret = clock_gettime(CLOCK_REALTIME, &end_ts);
    if (ret == -1) {
        EPRINTF("gettime error");
        goto err;
    }
    IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec\n", end_ts.tv_sec,
            end_ts.tv_nsec);

    /* compute time elapsed ms */
    elapsed_ts = timespec_sub(end_ts, start_ts);
    elapsed_nsec = timespec2nsec(elapsed_ts);

    printf("Sent %" PRIu64 " bytes to node %u in %3.3f s\n", total_bytes,
            dest_node, nsec2sec(elapsed_nsec));

    /* receive elapsed rx throughput time form dest_node */
    elapsed_rx_nsec = 0;
    pld_recv_size = sizeof(payload);
    err =  axiom_recv_raw(dev, &src_node, &port, &type, &pld_recv_size,
            &payload);
    if (err != AXIOM_RET_OK || (src_node != dest_node) ||
            payload.command != AXIOM_CMD_NETPERF_END) {
        EPRINTF("recv_elapsed_time error");
        goto err;
    }

    elapsed_rx_nsec = payload.elapsed_time;
    
    /* raw bytes include also the header */
    sent_raw_bytes = sent_bytes + (total_packets * sizeof(axiom_raw_hdr_t));

    tx_th = (double)(sent_bytes) / nsec2sec(elapsed_nsec);
    rx_th = (double)(sent_bytes) / nsec2sec(elapsed_rx_nsec);
    tx_raw_th = (double)(sent_raw_bytes) / nsec2sec(elapsed_nsec);
    rx_raw_th = (double)(sent_raw_bytes) / nsec2sec(elapsed_rx_nsec);
    tx_pps = (double)(total_packets) / nsec2sec(elapsed_nsec);
    rx_pps = (double)(total_packets) / nsec2sec(elapsed_rx_nsec);

    IPRINTF(verbose, "elapsed_tx_nsec = %" PRIu64 " - elapsed_rx_nsec = %"
            PRIu64, elapsed_nsec, elapsed_rx_nsec);

    printf("Throughput bytes/Sec    TX %3.3f (raw %3.3f) KB/s - "
            "RX %3.3f (raw %3.3f) KB/s\n",
            tx_th / 1024, tx_raw_th / 1024, rx_th / 1024, rx_raw_th / 1024);
    printf("Throughput packets/Sec  TX %3.3f Kpps - RX %3.3f Kpps\n",
            tx_pps / 1000, rx_pps / 1000);

err:
    axiom_close(dev);

    return 0;
}
