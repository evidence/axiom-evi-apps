/*
 * This file implements the axiom-netperf-reply application
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_small_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom_utility.h"
#include "dprintf.h"

#include "axiom_netperf_reply.h"

typedef struct axiom_netperf_status {
    struct timespec start_ts;
    uint64_t expected_bytes;
    uint64_t received_bytes;
} axiom_netperf_status_t;

axiom_netperf_status_t status[AXIOM_MAX_NODES];

static axiom_err_t
axiom_send_uint64_small(axiom_dev_t *dev, axiom_node_id_t dst,
        axiom_port_t port, axiom_flag_t flag, axiom_init_cmd_t cmd,
        uint64_t data)
{
    axiom_msg_id_t msg_err;
    axiom_netperf_payload_t payload;
    uint8_t i;
    uint8_t *data_p = ((uint8_t *)&data);

    payload.command = cmd;

    for (i = 0; i < sizeof(data); i += sizeof(payload.data)) {
        payload.offset = i;
        payload.data = *((typeof(payload.data) *)&data_p[i]);

        /* send small neighbour traceroute message */
        msg_err = axiom_send_small(dev, dst, port, flag,
                (axiom_payload_t *)&payload);
        if (msg_err == AXIOM_RET_ERROR) {
            return msg_err;
        }
        DPRINTF("payload - offset: 0x%x data: 0x%x", payload.offset,
                payload.data);
    }

    return AXIOM_RET_OK;
}

void
axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src,
        axiom_payload_t payload, int verbose)
{
    axiom_netperf_payload_t *recv_payload =
            ((axiom_netperf_payload_t *) &payload);
    axiom_netperf_status_t *cur_status = &status[src];
    struct timespec cur_ts;

    /* take a timestamp */
    if (clock_gettime(CLOCK_REALTIME, &cur_ts)) {
        EPRINTF("gettime error");
        return;
    }

    if (recv_payload->command == AXIOM_CMD_NETPERF_START) {
        /* TODO: define a macro */
        cur_status->expected_bytes = recv_payload->data <<
            recv_payload->offset;
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

    /* XXX tbv: does all 8 bytes of small messagge arrive? */
    cur_status->received_bytes += (uint16_t)sizeof(axiom_small_msg_t);

    DPRINTF("NETPERF msg received from: %u - expected_bytes: %llu "
            "received_bytes: %llu", src, cur_status->expected_bytes,
            cur_status->received_bytes);

    if (cur_status->received_bytes >= cur_status->expected_bytes)
    {
        axiom_err_t err;
        struct timespec elapsed_ts;
        uint64_t elapsed_nsec;
        double rx_th;

        /* get time of the last netperf message received */
        IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec\n", cur_ts.tv_sec,
                cur_ts.tv_nsec);

        /* compute time elapsed */
        elapsed_ts = timespec_sub(cur_ts, cur_status->start_ts);
        elapsed_nsec = timespec2nsec(elapsed_ts);
        rx_th = (double)(cur_status->received_bytes / nsec2sec(elapsed_nsec));
        IPRINTF(verbose, "Rx throughput = %3.3f KB/s - elapsed_nsec = %llu",
                rx_th / 1024, elapsed_nsec);

        /* send elapsed time to netperf application */
        err = axiom_send_uint64_small(dev, src, AXIOM_SMALL_PORT_NETUTILS,
                AXIOM_SMALL_FLAG_DATA, AXIOM_CMD_NETPERF_END, elapsed_nsec);
        if (err == AXIOM_RET_ERROR)
        {
            EPRINTF("send back time error");
            return;
        }
    }
}
