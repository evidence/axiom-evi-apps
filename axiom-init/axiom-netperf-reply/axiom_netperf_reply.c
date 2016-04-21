/*
 * This file implements the axiom-netperf-reply application
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_small_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "axiom_netperf_reply.h"

#define AXIOM_NETPERF_SCALE 10

typedef struct axiom_netperf_status {
    struct timeval start_tv;
    uint64_t expected_bytes;
    uint64_t received_bytes;
} axiom_netperf_status_t;

axiom_netperf_status_t status[AXIOM_MAX_NODES];

static double usec2sec(uint64_t usec)
{
    return ((double)(usec) / 1000000);
}

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
    struct timeval cur_tv;

    /* take a timestamp */
    if (gettimeofday(&cur_tv, NULL)) {
        EPRINTF("gettimeofday error");
        return;
    }

    if (recv_payload->command == AXIOM_CMD_NETPERF_START) {
        /* TODO: define a macro */
        cur_status->expected_bytes = recv_payload->data << AXIOM_NETPERF_SCALE;
        /* reset start time */
        //memset(&cur_status->start_tv, 0, sizeof(cur_status->start_tv));
        cur_status->received_bytes = 0;

        /* get time of the first netperf message received */
        memcpy(&cur_status->start_tv, &cur_tv, sizeof(cur_status->start_tv));
        IPRINTF(verbose,"Start timestamp: %ld sec\t%ld usec\n",
                cur_tv.tv_sec, cur_tv.tv_usec);
        return;
    } else if (recv_payload->command != AXIOM_CMD_NETPERF) {
        EPRINTF("receive a not AXIOM_CMD_NETPERF message");
        return;
    }

#if 0
    if ((cur_status->start_tv.tv_sec == 0) &&
            ((cur_status->start_tv.tv_usec == 0)))
    {
        /* get time of the first netperf message received */
        memcpy(&cur_status->start_tv, &cur_tv, sizeof(cur_status->start_tv));
        IPRINTF(verbose,"Start timestamp: %ld sec\t%ld usec\n",
                cur_tv.tv_sec, cur_tv.tv_usec);
    }
#endif

    /* XXX tbv: does all 8 bytes of small messagge arrive? */
    cur_status->received_bytes += (uint16_t)sizeof(axiom_small_msg_t);

    IPRINTF(verbose, "NETPERF msg received from: %u - expected_bytes: %llu \
            received_bytes: %llu", src, cur_status->expected_bytes,
            cur_status->received_bytes);

    if (cur_status->received_bytes >= cur_status->expected_bytes)
    {
        axiom_err_t err;
        struct timeval elapsed_tv;
        uint64_t elapsed_usec;
        double rx_th;

        /* get time of the last netperf message received */
        IPRINTF(verbose,"End timestamp: %ld sec\t%ld microsec\n", cur_tv.tv_sec,
                cur_tv.tv_usec);

        /* compute time elapsed */
        timersub(&cur_tv, &cur_status->start_tv, &elapsed_tv);
        elapsed_usec = elapsed_tv.tv_usec + (elapsed_tv.tv_sec * 1000000);
        rx_th = (double)(cur_status->received_bytes / usec2sec(elapsed_usec));
        IPRINTF(verbose, "Rx throughput = %3.3f KB/s - elapsed_usec = %llu",
                rx_th / 1024, elapsed_usec);

        /* send elapsed time to netperf application */
        err = axiom_send_uint64_small(dev, src, AXIOM_SMALL_PORT_NETUTILS,
                AXIOM_SMALL_FLAG_DATA, AXIOM_CMD_NETPERF_END, elapsed_usec);
        if (err == AXIOM_RET_ERROR)
        {
            EPRINTF("send back time error");
            return;
        }
    }
}
