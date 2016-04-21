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

#define AXIOM_NETPERF_SCALE 10

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-netperf [arguments] -d dest_node \n");
    printf("AXIOM netperf: estimate the throughput between this node and the\n");
    printf("               specified dest_node\n\n");
    printf("Arguments:\n");
    printf("-d, --dest      dest_node   destination node id of axiom-netperf\n");
    printf("-v, --verbose               verbose output\n");
    printf("-h, --help                  print this help\n\n");
}

static double
usec2sec(uint64_t usec)
{
    return ((double)(usec) / 1000000);
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
    struct timeval start_tv, end_tv, elapsed_tv;
    uint64_t elapsed_usec, elapsed_rx_usec;
    uint64_t sent_bytes, total_bytes = 5 << AXIOM_NETPERF_SCALE;
    double tx_th, rx_th;
    int dest_node_ok = 0, err, ret;
    int long_index = 0;
    int opt = 0;


    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv,"vhd:",
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

    total_bytes = (total_bytes >> AXIOM_NETPERF_SCALE) <<
        AXIOM_NETPERF_SCALE;

    /* send start message */
    payload.command = AXIOM_CMD_NETPERF_START;
    payload.data = total_bytes >> AXIOM_NETPERF_SCALE;
    msg_err = axiom_send_small(dev, dest_node, AXIOM_SMALL_PORT_INIT,
            AXIOM_SMALL_FLAG_DATA, (axiom_payload_t *)&payload);
    if (msg_err == AXIOM_RET_ERROR) {
        EPRINTF("send error");
        goto err;
    }
    printf("Starting send %llu bytes to node %u...\n", total_bytes, dest_node);

    /* get time of the first sent netperf message */
    ret = gettimeofday(&start_tv, NULL);
    if (ret == -1) {
        EPRINTF("gettimeofday error");
        goto err;
    }
    IPRINTF(verbose,"Start timestamp: %ld sec\t%ld microsec\n",
            start_tv.tv_sec, start_tv.tv_usec);

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
        IPRINTF(verbose, "NETPERF msg sent to: %u - total_bytes: %llu \
                sent_bytes: %llu", dest_node, total_bytes,
                sent_bytes + sizeof(axiom_small_msg_t));
    }

    /* get time of the last sent netperf message */
    ret = gettimeofday(&end_tv, NULL);
    if (ret == -1) {
        EPRINTF("gettimeofday error");
        goto err;
    }
    IPRINTF(verbose,"End timestamp: %ld sec\t%ld microsec\n", end_tv.tv_sec,
            end_tv.tv_usec);

    printf("Sent %llu bytes to node %u\n", total_bytes, dest_node);

    /* compute time elapsed ms */
    timersub(&end_tv, &start_tv, &elapsed_tv);
    elapsed_usec = elapsed_tv.tv_usec + (elapsed_tv.tv_sec * 1000000);

    /* receive elapsed rx throughput time form dest_node */
    elapsed_rx_usec = 0;
    err = axiom_recv_uint64_small(dev, &src_node, &port, &flag,
            AXIOM_CMD_NETPERF_END, &elapsed_rx_usec);
    if ((err == AXIOM_RET_ERROR) || (src_node != dest_node)) {
        EPRINTF("recv_elapsed_time error");
        goto err;
    }

    tx_th = (double)(sent_bytes / usec2sec(elapsed_usec));
    rx_th = (double)(sent_bytes / usec2sec(elapsed_rx_usec));

    IPRINTF(verbose, "elapsed_tx_usec = %llu - elapsed_rx_usec = %llu",
            elapsed_usec, elapsed_rx_usec);

    printf("Throughput TX %3.3f KB/s - RX %3.3f KB/s\n", tx_th / 1024,
            rx_th / 1024);

err:
    axiom_close(dev);

    return 0;
}
