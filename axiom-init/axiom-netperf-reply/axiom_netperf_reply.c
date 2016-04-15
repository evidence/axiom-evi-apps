/*
 * axiom_neteperf_reply.c
 *
 * Version:     v0.3.1
 * Last update: 2016-04-15
 *
 * This file implements the axiom-netperf-reply application
 *
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

static struct timeval start_tv = {
    .tv_sec = 0,
    .tv_usec = 0
};

static struct timeval end_tv, elapsed_tv;
static uint16_t rx_bytes = 0;

static int send_elapsed_time(axiom_dev_t *dev, axiom_node_id_t src,
                             uint64_t elapsed_usec) {

    axiom_msg_id_t msg_err;
    axiom_netperf_time_payload_t payload;
    int i;

    for (i = 0; i < (sizeof(elapsed_usec)/2); i++)
    {
        payload.command = AXIOM_CMD_NETPERF;
        payload.byte_order = i;
        payload.time = (uint16_t)(elapsed_usec >> (i*16));

        /* send small neighbour traceroute message */
        msg_err = axiom_send_small(dev, src, AXIOM_SMALL_PORT_NETUTILS, 0,
                                   (axiom_payload_t *)&payload);
        if (msg_err == AXIOM_RET_ERROR)
        {
            return -1;
        }
    }

    return 0;
}

void axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src,
        axiom_payload_t payload, int verbose) {

    uint64_t elapsed_usec;
    int err, ret;
    axiom_netperf_payload_t *recv_payload =
            ((axiom_netperf_payload_t *) &payload);

    if (recv_payload->command != AXIOM_CMD_NETPERF) {
        EPRINTF("receive a not AXIOM_CMD_NETPERF message");
        return;
    }

    if ((start_tv.tv_sec == 0) && ((start_tv.tv_usec == 0)))
    {
        /* get time of the first netperf message received */
        ret = gettimeofday(&start_tv, NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            return;
        }
        IPRINTF(verbose,"Start timestamp: %ld sec\t%ld microsec\n", start_tv.tv_sec,
                                                              start_tv.tv_usec);
    }
    IPRINTF(verbose, "NETPERF message received from: %u ", src);

    /* XXX tbv: does all 8 bytes of small messagge arrive? */
    rx_bytes += (uint16_t)sizeof(axiom_small_msg_t);

    if (recv_payload->total_bytes == rx_bytes)
    {
        /* get time of the last netperf message received */
        ret = gettimeofday(&end_tv, NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            return;
        }
        IPRINTF(verbose,"End timestamp: %ld sec\t%ld microsec\n", end_tv.tv_sec,
                                                              end_tv.tv_usec);
        /* compute time elapsed ms */
        timersub(&end_tv, &start_tv, &elapsed_tv);
        elapsed_usec = elapsed_tv.tv_usec + (elapsed_tv.tv_sec * 1000000);
        memset(&start_tv, 0, sizeof(start_tv)); /* reset start time */

        /* send elapsed time to netperf application */
        err = send_elapsed_time(dev, src, elapsed_usec);
        if (err == -1)
        {
            EPRINTF("send back time error");
            return;
        }

    }
}
