/*!
 * \file axiom_netperf_reply.c
 *
 * \version     v0.10
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the axiom-netperf messages.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

#include "../axiom-init.h"

/*! \brief axiom-netperf status */
typedef struct axiom_netperf_status {
    struct timespec start_ts;   /*!< \brief timestamp of the first byte */
    struct timespec cur_ts;     /*!< \brief current timestamp */
    uint64_t expected_bytes;    /*!< \brief total bytes that will be received */
    uint64_t received_bytes;    /*!< \brief number of bytes received */
} axiom_netperf_status_t;

/*! \brief axiom-netperf status array to handle all nodes */
axiom_netperf_status_t status[AXIOM_NODES_MAX];

void
axiom_netperf_send_reply(axiom_dev_t *dev, axiom_netperf_status_t *cur_status,
        axiom_node_id_t src, uint8_t error_report, int verbose)
{
        axiom_err_t err;
        axiom_netperf_payload_t payload;
        struct timespec elapsed_ts;
        uint64_t elapsed_nsec;
        double rx_th;

        IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec\n",
                cur_status->cur_ts.tv_sec, cur_status->cur_ts.tv_nsec);

        /* compute time elapsed */
        elapsed_ts = timespec_sub(cur_status->cur_ts, cur_status->start_ts);
        elapsed_nsec = timespec2nsec(elapsed_ts);
        rx_th = (double)(cur_status->received_bytes / nsec2sec(elapsed_nsec));
        IPRINTF(verbose, "Rx throughput = %3.3f KB/s - elapsed_nsec = %llu",
                rx_th / 1024, (long long unsigned)elapsed_nsec);

        /* send elapsed time to netperf application */
        payload.command = AXIOM_CMD_NETPERF_END;
        payload.total_bytes = cur_status->received_bytes;
        payload.elapsed_time = elapsed_nsec;
        payload.error = error_report;
        err = axiom_send_raw(dev, src, AXIOM_RAW_PORT_NETUTILS,
                AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
        if (!AXIOM_RET_IS_OK(err)) {
            EPRINTF("send back time error");
            return;
        }
}


void
axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src, size_t payload_size,
        void *payload, int verbose)
{
    axiom_netperf_payload_t *recv_payload =
            ((axiom_netperf_payload_t *) payload);
    axiom_netperf_status_t *cur_status = &status[src];

    /* take a timestamp */
    if (clock_gettime(CLOCK_REALTIME, &cur_status->cur_ts)) {
        EPRINTF("gettime error");
        return;
    }

    if (recv_payload->command == AXIOM_CMD_NETPERF_START) {
        /* TODO: define a macro */
        cur_status->expected_bytes = recv_payload->total_bytes;
        /* reset start time */
        //memset(&cur_status->start_ts, 0, sizeof(cur_status->start_ts));
        cur_status->received_bytes = 0;

        /* get time of the first netperf message received */
        memcpy(&cur_status->start_ts, &cur_status->cur_ts, sizeof(cur_status->start_ts));
        IPRINTF(verbose,"Start timestamp: %ld sec %ld nsec\n",
                cur_status->cur_ts.tv_sec, cur_status->cur_ts.tv_nsec);
        return;
    } else if (recv_payload->command == AXIOM_CMD_NETPERF_END) {
        int i, ret;
        uint8_t failed = 0;
        void *rdma_zone;
        uint64_t rdma_size;

        /* check RDMA */
        if (recv_payload->type != AXNP_RDMA)
            return;

        /* RDMA end message */
        cur_status->received_bytes = recv_payload->total_bytes;

        /* map RDMA zone */
        rdma_zone = axiom_rdma_mmap(dev, &rdma_size);
        if (!rdma_zone) {
            EPRINTF("rdma map failed");
            failed = 1;
        }

        if (!failed) {
            /* TODO: check magic */
            for (i = 0; i < cur_status->received_bytes; i++) {
                ret = memcmp(rdma_zone + i, &recv_payload->magic,
                        sizeof(recv_payload->magic));
                if (ret) {
                    failed = 1;
                    break;
                }
            }
            axiom_rdma_munmap(dev);
        }

        axiom_netperf_send_reply(dev, cur_status, src, failed, verbose);
    } else if (recv_payload->command == AXIOM_CMD_NETPERF) {
        /* RAW message */
        cur_status->received_bytes += payload_size;

        DPRINTF("NETPERF msg received from: %u - expected_bytes: %llu "
                "received_bytes: %llu", src,
                (long long unsigned)cur_status->expected_bytes,
                (long long unsigned)cur_status->received_bytes);

        if (cur_status->received_bytes >= cur_status->expected_bytes)
            axiom_netperf_send_reply(dev, cur_status, src, 0, verbose);
    } else {
        EPRINTF("receive a not AXIOM_CMD_NETPERF message");
        return;
    }
}
