/*!
 * \file axiom-netperf.c
 *
 * \version     v0.11
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-netperf application.
 *
 * axiom-netperf estimates the throughput between two axiom nodes.
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

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

#define AXIOM_NETPERF_DEF_CHAR_SCALE    'B'
#define AXIOM_NETPERF_DEF_DATA_SCALE    10
#define AXIOM_NETPERF_DEF_DATA_LENGTH   1024
#define AXIOM_NETPERF_DEF_RDMA_PSIZE    AXIOM_RDMA_PAYLOAD_MAX_SIZE
#define AXIOM_NETPERF_DEF_RAW_PSIZE     AXIOM_RAW_PAYLOAD_MAX_SIZE
#define AXIOM_NETPERF_DEF_LONG_PSIZE    AXIOM_LONG_PAYLOAD_MAX_SIZE
#define AXIOM_NETPERF_DEF_TYPE          AXNP_LONG

typedef struct axnetperf_status {
    axiom_dev_t *dev;
    axiom_netperf_type_t np_type;
    axiom_node_id_t dest_node;
    struct timespec start_ts;
    struct timespec end_ts;
    size_t  payload_size;
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t sent_bytes;
    uint64_t sent_raw_bytes;
    void *rdma_zone;
    uint64_t rdma_size;
    uint8_t magic;

} axnetperf_status_t;

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-netperf [arguments] -d dest_node \n");
    printf("AXIOM netperf: estimate the throughput between this node and the\n");
    printf("               specified dest_node\n\n");
    printf("Arguments:\n");
    printf("-t, --type      raw|rdma|long  message type to use [default: LONG]\n");
    printf("-d, --dest      dest_node      destination node id of axiom-netperf\n");
    printf("-l, --length    x[B|K|M|G]     bytes to send to the destination node\n");
    printf("                               The suffix specifies the length unit\n");
    printf("-p, --payload   size           payload size in bytes [default: "
            "raw - %d rdma - %d long - %d]\n",
            AXIOM_NETPERF_DEF_RAW_PSIZE, AXIOM_NETPERF_DEF_RDMA_PSIZE,
            AXIOM_NETPERF_DEF_LONG_PSIZE);
    printf("-v, --verbose                  verbose output\n");
    printf("-h, --help                     print this help\n\n");
}

static int
get_scale(char char_scale) {
    int scale;

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

inline static void
axnetperf_start_time(axnetperf_status_t *s)
{
    int ret;
    ret = clock_gettime(CLOCK_REALTIME, &s->start_ts);
    IPRINTF(verbose,"Start timestamp: %ld sec %ld nanosec - ret: %d\n",
            s->start_ts.tv_sec, s->start_ts.tv_nsec, ret);
}

inline static void
axnetperf_end_time(axnetperf_status_t *s)
{
    int ret;
    ret = clock_gettime(CLOCK_REALTIME, &s->end_ts);
    IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec - ret: %d\n",
            s->end_ts.tv_sec, s->end_ts.tv_nsec, ret);
}

static int
axnetperf_raw_init(axnetperf_status_t *s)
{
    /* set default payload size if it is unspecified */
    if (s->payload_size == 0) {
        s->payload_size = AXIOM_NETPERF_DEF_RAW_PSIZE;
    }

    if (s->payload_size < 1 || s->payload_size > AXIOM_RAW_PAYLOAD_MAX_SIZE) {
        EPRINTF("RAW payload size must be between 1 and %d bytes",
                AXIOM_RAW_PAYLOAD_MAX_SIZE);
        return -1;
    }

    return 0;
}

static int
axnetperf_long_init(axnetperf_status_t *s)
{
    /* set default payload size if it is unspecified */
    if (s->payload_size == 0) {
        s->payload_size = AXIOM_NETPERF_DEF_LONG_PSIZE;
    }

    if (s->payload_size < 1 || s->payload_size > AXIOM_LONG_PAYLOAD_MAX_SIZE) {
        EPRINTF("LONG payload size must be between 1 and %d bytes",
                AXIOM_LONG_PAYLOAD_MAX_SIZE);
        return -1;
    }

    return 0;
}

static int
axnetperf_raw_long(axnetperf_status_t *s)
{
    axiom_netperf_payload_t payload;
    axiom_long_payload_t long_payload;
    axiom_err_t err;

    payload.command = AXIOM_CMD_NETPERF;

    memcpy(&long_payload, &payload, sizeof(payload));

    /* get time of the first sent netperf message */
    axnetperf_start_time(s);

    for (s->sent_bytes = 0; s->sent_bytes < s->total_bytes;
            s->sent_bytes += s->payload_size) {

        /* send netperf message */
        if (s->np_type == AXNP_RAW) {
            err = axiom_send_raw(s->dev, s->dest_node, AXIOM_RAW_PORT_INIT,
                    AXIOM_TYPE_RAW_DATA, s->payload_size, &payload);
        } else {
            err = axiom_send_long(s->dev, s->dest_node, AXIOM_RAW_PORT_INIT,
                    s->payload_size, &long_payload);
        }

        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            EPRINTF("send error");
            return err;
        }

        s->total_packets++;

        DPRINTF("NETPERF msg sent to: %u - total_bytes: %" PRIu64
                " sent_bytes: %" PRIu64, s->dest_node, s->total_bytes,
                s->sent_bytes + sizeof(axiom_raw_msg_t));
    }

    /* get time of the last sent netperf message */
    axnetperf_end_time(s);

    /* raw bytes include also the header */
    s->sent_raw_bytes = s->sent_bytes +
        (s->total_packets * sizeof(axiom_raw_hdr_t));

    return 0;
}

static int
axnetperf_rdma_init(axnetperf_status_t *s)
{

    /* set default payload size if it is unspecified */
    if (s->payload_size == 0) {
        s->payload_size = AXIOM_NETPERF_DEF_RDMA_PSIZE;
    }

    if (s->payload_size < 1 ||
            s->payload_size > AXIOM_RDMA_PAYLOAD_MAX_SIZE) {
        EPRINTF("RDMA payload size must be between 1 and %d bytes",
                AXIOM_RDMA_PAYLOAD_MAX_SIZE);
        return -1;
    }

    if (s->payload_size & 0x7) {
        EPRINTF("RDMA payload size must be aligned to 8 bytes");
        return -1;
    }

    /* map rdma zone */
    s->rdma_zone = axiom_rdma_mmap(s->dev, &s->rdma_size);
    if (!s->rdma_zone) {
        EPRINTF("rdma map failed");
        return -1;
    }

    if (s->total_bytes > s->rdma_size) {
        EPRINTF("Out of RDMA zone - rdma_size: %" PRIu64, s->rdma_size);
        return -1;
    }

    /* fill the rdma zone with magic value */
    memset(s->rdma_zone, s->magic, s->total_bytes);

    IPRINTF(verbose, "rdma_mmap - addr: %p size: %" PRIu64,
            s->rdma_zone, s->rdma_size);
    return 0;
}

static int
axnetperf_rdma(axnetperf_status_t *s)
{
    axiom_netperf_payload_t payload;
    int payload_size = s->payload_size;
    axiom_err_t err;

    /* get time of the first sent netperf message */
    axnetperf_start_time(s);

    for (s->sent_bytes = 0; s->sent_bytes < s->total_bytes;
            s->sent_bytes += payload_size) {

#if 0
        if ((s->total_bytes - s->sent_bytes) < payload_size) {
            payload_size = s->total_bytes - s->sent_bytes;
            rdma_psize = payload_size >> AXIOM_RDMA_PAYLOAD_SIZE_ORDER;
        }

        /* write payload to remote node */
        err = axiom_rdma_write_sync(s->dev, s->dest_node, AXIOM_RAW_PORT_INIT,
                rdma_psize, s->sent_bytes, s->sent_bytes);
        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            EPRINTF("send error");
            return err;
        }
#endif

        s->total_packets++;
    }

    if ((s->total_bytes - s->sent_bytes) > 0) {
#if 0
        rdma_psize = (s->total_bytes - s->sent_bytes)
            >> AXIOM_RDMA_PAYLOAD_SIZE_ORDER;

        /* write payload to remote node */
        err = axiom_rdma_write_sync(s->dev, s->dest_node, AXIOM_RAW_PORT_INIT,
                rdma_psize, s->sent_bytes, s->sent_bytes);
        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            EPRINTF("send error");
            return err;
        }
#endif

        s->total_packets++;
    }

    /* get time of the last sent netperf message */
    axnetperf_end_time(s);

    /* TODO: raw bytes include also the header */
    s->sent_raw_bytes = s->sent_bytes;

    /* send end message to the slave */
    payload.command = AXIOM_CMD_NETPERF_END;
    payload.total_bytes = s->sent_bytes;
    payload.type = s->np_type;
    payload.magic = s->magic;

    err = axiom_send_raw(s->dev, s->dest_node, AXIOM_RAW_PORT_INIT,
            AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
    if (unlikely(!AXIOM_RET_IS_OK(err))) {
        EPRINTF("send error");
        return err;
    }

    return 0;
}

static int
axnetperf_start(axnetperf_status_t *s)
{
    axiom_netperf_payload_t payload;
    axiom_err_t err;

    payload.command = AXIOM_CMD_NETPERF_START;
    payload.total_bytes = s->total_bytes;
    payload.type = s->np_type;
    payload.magic = s->magic;

    err = axiom_send_raw(s->dev, s->dest_node, AXIOM_RAW_PORT_INIT,
            AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
    if (unlikely(!AXIOM_RET_IS_OK(err))) {
        EPRINTF("send error");
        return err;
    }

    printf("Starting axiom-netperf to node %u\n", s->dest_node);
    if (s->np_type == AXNP_RAW)
        printf("   message type: RAW\n");
    else if (s->np_type == AXNP_RDMA)
        printf("   message type: RDMA\n");
    else if (s->np_type == AXNP_LONG)
        printf("   message type: LONG\n");
    printf("   payload size: %zu bytes\n", s->payload_size);
    printf("   total bytes: %" PRIu64 " bytes\n", s->total_bytes);
    printf("   magic number: %" PRIu8 "\n", s->magic);

    return 0;
}

static int
axnetperf_stop(axnetperf_status_t *s)
{
    axiom_raw_payload_size_t pld_recv_size;
    axiom_netperf_payload_t payload;
    axiom_node_id_t src_node;
    axiom_port_t port;
    axiom_type_t type;
    axiom_err_t err;
    double tx_th, rx_th, tx_raw_th, rx_raw_th, tx_pps, rx_pps;
    uint64_t elapsed_nsec, elapsed_rx_nsec;
    struct timespec elapsed_ts;

    /* compute time elapsed ms */
    elapsed_ts = timespec_sub(s->end_ts, s->start_ts);
    elapsed_nsec = timespec2nsec(elapsed_ts);

    printf("Sent %" PRIu64 " bytes to node %u in %3.3f s\n", s->total_bytes,
            s->dest_node, nsec2sec(elapsed_nsec));

    /* receive elapsed rx throughput time form dest_node */
    elapsed_rx_nsec = 0;
    pld_recv_size = sizeof(payload);
    err =  axiom_recv_raw(s->dev, &src_node, &port, &type, &pld_recv_size,
            &payload);
    if (!AXIOM_RET_IS_OK(err) || (src_node != s->dest_node) ||
            payload.command != AXIOM_CMD_NETPERF_END) {
        EPRINTF("recv_elapsed_time error - err: 0x%x node: 0x%x [0x%x] "
                "command 0x%x [0x%x]", err, src_node, s->dest_node,
                payload.command, AXIOM_CMD_NETPERF_END);
        return -1;
    }

    elapsed_rx_nsec = payload.elapsed_time;

    tx_th = (double)(s->sent_bytes) / nsec2sec(elapsed_nsec);
    rx_th = (double)(s->sent_bytes) / nsec2sec(elapsed_rx_nsec);
    tx_raw_th = (double)(s->sent_raw_bytes) / nsec2sec(elapsed_nsec);
    rx_raw_th = (double)(s->sent_raw_bytes) / nsec2sec(elapsed_rx_nsec);
    tx_pps = (double)(s->total_packets) / nsec2sec(elapsed_nsec);
    rx_pps = (double)(s->total_packets) / nsec2sec(elapsed_rx_nsec);

    IPRINTF(verbose, "elapsed_tx_nsec = %" PRIu64 " - elapsed_rx_nsec = %"
            PRIu64, elapsed_nsec, elapsed_rx_nsec);

    printf("Throughput bytes/Sec    TX %3.3f (raw %3.3f) KB/s - "
            "RX %3.3f (raw %3.3f) KB/s\n",
            tx_th / 1024, tx_raw_th / 1024, rx_th / 1024, rx_raw_th / 1024);
    printf("Throughput packets/Sec  TX %3.3f Kpps - RX %3.3f Kpps\n",
            tx_pps / 1000, rx_pps / 1000);

    if (payload.error) {
        printf("\n Remote node reports some ERRORS [%u]", payload.error);
    }

    return 0;
}

int
main(int argc, char **argv)
{
    uint32_t data_length = AXIOM_NETPERF_DEF_DATA_LENGTH;
    int data_scale = AXIOM_NETPERF_DEF_DATA_SCALE;
    axnetperf_status_t s = {
        .dev = NULL,
        .dest_node = AXIOM_NULL_NODE,
        .np_type = AXIOM_NETPERF_DEF_TYPE,
        .payload_size = 0,
    };
    axiom_err_t err;
    int ret;

    int long_index, opt;
    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"type", required_argument, 0, 't'},
        {"length", required_argument, 0, 'l'},
        {"payload", required_argument, 0, 'p'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"vhd:l:p:t:",
                         long_options, &long_index )) != -1) {
        char *type_string = NULL;
        char char_scale = AXIOM_NETPERF_DEF_CHAR_SCALE;

        switch (opt) {
            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &s.dest_node) != 1) {
                    EPRINTF("wrong number of destination nodes");
                    usage();
                    exit(-1);
                }
                break;

            case 't':
                if ((sscanf(optarg, "%ms", &type_string) != 1) ||
                        type_string == NULL) {
                    EPRINTF("wrong message type");
                    usage();
                    exit(-1);
                }

                if (strncmp(type_string, "rdma", 4) == 0) {
                    s.np_type = AXNP_RDMA;
                } else if (strncmp(type_string, "long", 4) == 0) {
                    s.np_type = AXNP_LONG;
                } else if (strncmp(type_string, "raw", 3) == 0) {
                    s.np_type = AXNP_RAW;
                } else {
                    EPRINTF("wrong message type");
                    free(type_string);
                    usage();
                    exit(-1);
                }

                free(type_string);
                break;

            case 'l' :
                if (sscanf(optarg, "%" SCNu32 "%c", &data_length, &char_scale) == 0) {
                    EPRINTF("wrong number of length");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                break;

            case 'p' :
                if (sscanf(optarg, "%zu", &s.payload_size) != 1) {
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
    if (s.dest_node == AXIOM_NULL_NODE) {
        EPRINTF("You must specify the destination node")
        usage();
        return 0;
    }

    /* open the axiom device */
    s.dev = axiom_open(NULL);
    if (s.dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    s.total_bytes = data_length << data_scale;

    /* generate random magic */
    srand(time(NULL));
    s.magic = rand() % 255;

    ret = 0;
    /* init subsystems */
    switch (s.np_type) {
        case AXNP_RDMA:
            ret = axnetperf_rdma_init(&s);
            break;

        case AXNP_RAW:
            ret = axnetperf_raw_init(&s);
            break;

        case AXNP_LONG:
            ret = axnetperf_long_init(&s);
            break;

        default:
            EPRINTF("axiom-netperf type invalid");
            ret = -1;
    }

    if (ret) {
        EPRINTF("init failed");
        goto err;
    }

    /* bind the current process on port */
    err = axiom_bind(s.dev, AXIOM_RAW_PORT_NETUTILS);
    if (err != AXIOM_RAW_PORT_NETUTILS) {
        EPRINTF("axiom_bind error");
        goto err;
    }

    /* send start message */
    ret = axnetperf_start(&s);
    if (ret) {
        goto err;
    }

    switch (s.np_type) {
        case AXNP_RDMA:
            ret = axnetperf_rdma(&s);
            break;

        case AXNP_RAW:
        case AXNP_LONG:
            ret = axnetperf_raw_long(&s);
            break;

        default:
            EPRINTF("axiom-netperf type invalid");
            goto err;
    }

    /* receive end message */
    ret = axnetperf_stop(&s);
    if (ret) {
        goto err;
    }

err:
    axiom_close(s.dev);

    return 0;
}
