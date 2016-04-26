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

/* receive elapsed time from remote node */
static axiom_err_t
axiom_recv_uint64_small(axiom_dev_t *dev, axiom_node_id_t *src,
        axiom_port_t *port, axiom_flag_t *flag, axiom_init_cmd_t cmd,
        uint64_t *data)
{
    axiom_msg_id_t msg_err;
    axiom_netperf_payload_t payload;
    uint8_t i;
    uint8_t *data_p = ((uint8_t *)data);

    for (i = 0; i < sizeof(*data); i += sizeof(payload.data)) {
        msg_err =  axiom_recv_small(dev, src, port, flag,
                (axiom_payload_t *)&payload);
        if (msg_err == AXIOM_RET_ERROR || cmd != payload.command) {
            return AXIOM_RET_ERROR;
        }
        *((typeof(payload.data) *)&data_p[payload.offset]) = payload.data;
        DPRINTF("payload - offset: 0x%x data: 0x%x", payload.offset, payload.data);
    }

    return AXIOM_RET_OK;
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t msg_err;
    axiom_node_id_t dest_node, src_node;
    axiom_port_t port;
    axiom_flag_t flag;
    axiom_netperf_payload_t payload;
    struct timespec start_ts, end_ts, elapsed_ts;
    double tx_th, rx_th;
    int dest_node_ok = 0, err, ret, long_index, opt;
    uint64_t elapsed_nsec, elapsed_rx_nsec, sent_bytes, total_bytes;
    uint16_t data_length = AXIOM_NETPERF_DEF_DATA_LENGTH;
    uint8_t data_scale = AXIOM_NETPERF_DEF_DATA_SCALE;
    char char_scale = AXIOM_NETPERF_DEF_CHAR_SCALE;

    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"length", required_argument, 0, 'l'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"vhd:l:",
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

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    /* bind the current process on port */
    err = axiom_bind(dev, AXIOM_SMALL_PORT_NETUTILS);
    if (err == AXIOM_RET_ERROR) {
        EPRINTF("axiom_bind error");
        goto err;
    }

    total_bytes = data_length << data_scale;

    /* send start message */
    payload.command = AXIOM_CMD_NETPERF_START;
    payload.data = data_length;
    payload.offset = data_scale;
    msg_err = axiom_send_small(dev, dest_node, AXIOM_SMALL_PORT_INIT,
            AXIOM_SMALL_FLAG_DATA, (axiom_payload_t *)&payload);
    if (msg_err == AXIOM_RET_ERROR) {
        EPRINTF("send error");
        goto err;
    }
    printf("Starting send %" PRIu64 " bytes to node %u...\n", total_bytes, dest_node);

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
            sent_bytes += sizeof(axiom_small_msg_t)) {
        /* send netperf message */
        msg_err = axiom_send_small(dev, dest_node, AXIOM_SMALL_PORT_INIT,
                AXIOM_SMALL_FLAG_DATA, (axiom_payload_t *)&payload);
        if (msg_err == AXIOM_RET_ERROR) {
            EPRINTF("send error");
            goto err;
        }
        DPRINTF("NETPERF msg sent to: %u - total_bytes: %" PRIu64
                " sent_bytes: %" PRIu64, dest_node, total_bytes,
                sent_bytes + sizeof(axiom_small_msg_t));
    }

    /* get time of the last sent netperf message */
    ret = clock_gettime(CLOCK_REALTIME, &end_ts);
    if (ret == -1) {
        EPRINTF("gettime error");
        goto err;
    }
    IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec\n", end_ts.tv_sec,
            end_ts.tv_nsec);

    printf("Sent %" PRIu64 " bytes to node %u\n", total_bytes, dest_node);

    /* compute time elapsed ms */
    elapsed_ts = timespec_sub(end_ts, start_ts);
    elapsed_nsec = timespec2nsec(elapsed_ts);

    /* receive elapsed rx throughput time form dest_node */
    elapsed_rx_nsec = 0;
    err = axiom_recv_uint64_small(dev, &src_node, &port, &flag,
            AXIOM_CMD_NETPERF_END, &elapsed_rx_nsec);
    if ((err == AXIOM_RET_ERROR) || (src_node != dest_node)) {
        EPRINTF("recv_elapsed_time error");
        goto err;
    }

    tx_th = (double)(sent_bytes / nsec2sec(elapsed_nsec));
    rx_th = (double)(sent_bytes / nsec2sec(elapsed_rx_nsec));

    IPRINTF(verbose, "elapsed_tx_nsec = %" PRIu64 " - elapsed_rx_nsec = %"
            PRIu64, elapsed_nsec, elapsed_rx_nsec);

    printf("Throughput TX %3.3f KB/s - RX %3.3f KB/s\n", tx_th / 1024,
            rx_th / 1024);

err:
    axiom_close(dev);

    return 0;
}
