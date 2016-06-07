/*!
 * \file axiom_netperf_reply.c
 *
 * \version     v0.5
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
    uint64_t expected_bytes;    /*!< \brief total bytes that will be received */
    uint64_t received_bytes;    /*!< \brief number of bytes received */
} axiom_netperf_status_t;

/*! \brief axiom-netperf status array to handle all nodes */
axiom_netperf_status_t status[AXIOM_NODES_MAX];

void
axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src, axiom_payload_size_t
        payload_size, axiom_init_payload_t *payload, int verbose)
{
    axiom_netperf_payload_t *recv_payload =
            ((axiom_netperf_payload_t *) payload);
    axiom_netperf_status_t *cur_status = &status[src];
    struct timespec cur_ts;

    /* take a timestamp */
    if (clock_gettime(CLOCK_REALTIME, &cur_ts)) {
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
        memcpy(&cur_status->start_ts, &cur_ts, sizeof(cur_status->start_ts));
        IPRINTF(verbose,"Start timestamp: %ld sec %ld nsec\n",
                cur_ts.tv_sec, cur_ts.tv_nsec);
        return;
    } else if (recv_payload->command != AXIOM_CMD_NETPERF) {
        EPRINTF("receive a not AXIOM_CMD_NETPERF message");
        return;
    }

#if 0
    if ((cur_status->start_ts.tv_sec == 0) &&
            ((cur_status->start_ts.tv_nsec == 0)))
    {
        /* get time of the first netperf message received */
        memcpy(&cur_status->start_ts, &cur_ts, sizeof(cur_status->start_ts));
        IPRINTF(verbose,"Start timestamp: %ld sec\t%ld nsec\n",
                cur_ts.tv_sec, cur_ts.tv_nsec);
    }
#endif

    /* XXX tbv: does all 8 bytes of raw messagge arrive? */
    cur_status->received_bytes += payload_size;

    DPRINTF("NETPERF msg received from: %u - expected_bytes: %llu "
            "received_bytes: %llu", src,
            (long long unsigned)cur_status->expected_bytes,
            (long long unsigned)cur_status->received_bytes);

    if (cur_status->received_bytes >= cur_status->expected_bytes)
    {
        axiom_err_t err;
        axiom_netperf_payload_t payload;
        struct timespec elapsed_ts;
        uint64_t elapsed_nsec;
        double rx_th;

        IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec\n", cur_ts.tv_sec,
                cur_ts.tv_nsec);

        /* compute time elapsed */
        elapsed_ts = timespec_sub(cur_ts, cur_status->start_ts);
        elapsed_nsec = timespec2nsec(elapsed_ts);
        rx_th = (double)(cur_status->received_bytes / nsec2sec(elapsed_nsec));
        IPRINTF(verbose, "Rx throughput = %3.3f KB/s - elapsed_nsec = %llu",
                rx_th / 1024, (long long unsigned)elapsed_nsec);

        /* send elapsed time to netperf application */
        payload.command = AXIOM_CMD_NETPERF_END;
        payload.total_bytes = cur_status->received_bytes;
        payload.elapsed_time = elapsed_nsec;
        err = axiom_send_raw(dev, src, AXIOM_RAW_PORT_NETUTILS,
                AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
        if (err == AXIOM_RET_ERROR) {
            EPRINTF("send back time error");
            return;
        }
    }
}
