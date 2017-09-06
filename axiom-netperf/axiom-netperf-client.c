/*!
 * \file axiom-netperf.c
 *
 * \version     v0.13
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

#include "axiom-netperf.h"

extern int verbose;

inline static void
axnetperf_start_time(axnetperf_status_t *s)
{
    int ret;
    ret = clock_gettime(CLOCK_REALTIME, &s->client.start_ts);
    IPRINTF(verbose,"Start timestamp: %ld sec %ld nanosec - ret: %d\n",
            s->client.start_ts.tv_sec, s->client.start_ts.tv_nsec, ret);
}

inline static void
axnetperf_end_time(axnetperf_status_t *s)
{
    int ret;
    ret = clock_gettime(CLOCK_REALTIME, &s->client.end_ts);
    IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec - ret: %d\n",
            s->client.end_ts.tv_sec, s->client.end_ts.tv_nsec, ret);
}

static int
axnetperf_raw_init(axnetperf_status_t *s)
{
    /* set default payload size if it is unspecified */
    if (s->client.payload_size == 0) {
        s->client.payload_size = AXIOM_NETPERF_DEF_RAW_PSIZE;
    }

    if (s->client.payload_size < 1 || s->client.payload_size > AXIOM_RAW_PAYLOAD_MAX_SIZE) {
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
    if (s->client.payload_size == 0) {
        s->client.payload_size = AXIOM_NETPERF_DEF_LONG_PSIZE;
    }

    if (s->client.payload_size < 1 || s->client.payload_size > AXIOM_LONG_PAYLOAD_MAX_SIZE) {
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

    for (s->client.sent_bytes = 0; s->client.sent_bytes < s->client.total_bytes;
            s->client.sent_bytes += s->client.payload_size) {

        /* send netperf message */
        if (s->np_type == AXNP_RAW) {
            err = axiom_send_raw(s->dev, s->server_id, s->server_port,
                    AXIOM_TYPE_RAW_DATA, s->client.payload_size, &payload);
        } else {
            err = axiom_send_long(s->dev, s->server_id, s->server_port,
                    s->client.payload_size, &long_payload);
        }

        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            EPRINTF("send error");
            return err;
        }

        s->client.total_packets++;

        DPRINTF("NETPERF msg sent to: %u - total_bytes: %" PRIu64
                " sent_bytes: %" PRIu64, s->server_id, s->client.total_bytes,
                s->client.sent_bytes + sizeof(axiom_raw_msg_t));
    }

    /* get time of the last sent netperf message */
    axnetperf_end_time(s);

    /* raw bytes include also the header */
    s->client.sent_raw_bytes = s->client.sent_bytes +
        (s->client.total_packets * sizeof(axiom_raw_hdr_t));

    return 0;
}

static int
axnetperf_rdma_init(axnetperf_status_t *s)
{

    /* set default payload size if it is unspecified */
    if (s->client.payload_size == 0) {
        s->client.payload_size = AXIOM_NETPERF_DEF_RDMA_PSIZE;
    }

    if (s->client.payload_size < 1 ||
            s->client.payload_size > AXIOM_RDMA_PAYLOAD_MAX_SIZE) {
        EPRINTF("RDMA payload size must be between 1 and %d bytes",
                AXIOM_RDMA_PAYLOAD_MAX_SIZE);
        return -1;
    }

    /* map rdma zone */
    s->client.rdma_zone = axiom_rdma_mmap(s->dev, &s->client.rdma_size);
    if (!s->client.rdma_zone) {
        EPRINTF("rdma map failed");
        return -1;
    }

    if (s->client.total_bytes > s->client.rdma_size) {
        EPRINTF("Out of RDMA zone - rdma_size: %" PRIu64, s->client.rdma_size);
        return -1;
    }

    /* fill the rdma zone with magic value */
    memset(s->client.rdma_zone, s->client.magic, s->client.total_bytes);

    IPRINTF(verbose, "rdma_mmap - addr: %p size: %" PRIu64,
            s->client.rdma_zone, s->client.rdma_size);
    return 0;
}

#define TOKEN_LEN 2048

static axiom_err_t
axnetperf_rdma_async(axnetperf_status_t *s)
{
    int payload_size = s->client.payload_size, i = 0;
    axiom_err_t err = AXIOM_RET_OK;
    axiom_token_t tokens[TOKEN_LEN];

    for (s->client.sent_bytes = 0; s->client.sent_bytes < s->client.total_bytes;
            s->client.sent_bytes += payload_size) {

        if ((s->client.total_bytes - s->client.sent_bytes) < payload_size) {
            payload_size = s->client.total_bytes - s->client.sent_bytes;
        }

        /* write payload to remote node */
        err = axiom_rdma_write(s->dev, s->server_id, payload_size,
                (void *) s->client.sent_bytes, (void *) s->client.sent_bytes, &tokens[i]);
        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            return err;
        }

        s->client.total_packets++;
        i++;
        if (i == TOKEN_LEN) {
            axiom_rdma_wait(s->dev, tokens, i);
            i = 0;
        }
    }

    if ((s->client.total_bytes - s->client.sent_bytes) > 0) {
        payload_size = s->client.total_bytes - s->client.sent_bytes;

        /* write payload to remote node */
        err = axiom_rdma_write_sync(s->dev, s->server_id, payload_size,
                (void *) s->client.sent_bytes, (void *) s->client.sent_bytes, &tokens[i]);
        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            return err;
        }

        s->client.total_packets++;
        i++;
    }

    err = axiom_rdma_wait(s->dev, tokens, i);

    return err;
}

static axiom_err_t
axnetperf_rdma_sync(axnetperf_status_t *s)
{
    int payload_size = s->client.payload_size;
    axiom_err_t err = AXIOM_RET_OK;

    for (s->client.sent_bytes = 0; s->client.sent_bytes < s->client.total_bytes;
            s->client.sent_bytes += payload_size) {

        if ((s->client.total_bytes - s->client.sent_bytes) < payload_size) {
            payload_size = s->client.total_bytes - s->client.sent_bytes;
        }

        /* write payload to remote node */
        err = axiom_rdma_write_sync(s->dev, s->server_id, payload_size,
                (void *) s->client.sent_bytes, (void *) s->client.sent_bytes, NULL);
        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            return err;
        }

        s->client.total_packets++;
    }

    if ((s->client.total_bytes - s->client.sent_bytes) > 0) {
        payload_size = s->client.total_bytes - s->client.sent_bytes;

        /* write payload to remote node */
        err = axiom_rdma_write_sync(s->dev, s->server_id, payload_size,
                (void *) s->client.sent_bytes, (void *) s->client.sent_bytes, NULL);
        if (unlikely(!AXIOM_RET_IS_OK(err))) {
            return err;
        }

        s->client.total_packets++;
    }

    return err;
}

static int
axnetperf_rdma(axnetperf_status_t *s)
{
    axiom_netperf_payload_t payload;
    axiom_err_t err;

    /* get time of the first sent netperf message */
    axnetperf_start_time(s);

    if (s->client.rdma_sync)
        err = axnetperf_rdma_sync(s);
    else
        err = axnetperf_rdma_async(s);

    if (unlikely(!AXIOM_RET_IS_OK(err))) {
        EPRINTF("send error");
        return err;
    }

    /* get time of the last sent netperf message */
    axnetperf_end_time(s);

    /* raw bytes include also the header */
    s->client.sent_raw_bytes = s->client.sent_bytes +
        (s->client.total_packets * sizeof(axiom_rdma_hdr_t));

    /* send end message to the slave */
    payload.command = AXIOM_CMD_NETPERF_END;
    payload.total_bytes = s->client.sent_bytes;
    payload.type = s->np_type;
    payload.magic = s->client.magic;

    err = axiom_send_raw(s->dev, s->server_id, s->server_port,
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
    payload.total_bytes = s->client.total_bytes;
    payload.type = s->np_type;
    payload.reply_port = s->client_port;
    payload.magic = s->client.magic;

    err = axiom_send_raw(s->dev, s->server_id, s->server_port,
            AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
    if (unlikely(!AXIOM_RET_IS_OK(err))) {
        EPRINTF("send error");
        return err;
    }

    printf("Starting axiom-netperf to node %u\n", s->server_id);
    if (s->np_type == AXNP_RAW)
        printf("   message type: RAW\n");
    else if (s->np_type == AXNP_RDMA)
        printf("   message type: RDMA\n");
    else if (s->np_type == AXNP_LONG)
        printf("   message type: LONG\n");
    printf("   payload size: %zu bytes\n", s->client.payload_size);
    printf("   total bytes: %" PRIu64 " bytes\n", s->client.total_bytes);
    printf("   magic number: %" PRIu8 "\n", s->client.magic);

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
    elapsed_ts = timespec_sub(s->client.end_ts, s->client.start_ts);
    elapsed_nsec = timespec2nsec(elapsed_ts);

    printf("Sent %" PRIu64 " bytes to node %u in %3.3f s\n", s->client.total_bytes,
            s->server_id, nsec2sec(elapsed_nsec));

    tx_th = (double)(s->client.sent_bytes) / nsec2sec(elapsed_nsec);
    tx_raw_th = (double)(s->client.sent_raw_bytes) / nsec2sec(elapsed_nsec);
    tx_pps = (double)(s->client.total_packets) / nsec2sec(elapsed_nsec);

    printf("Throughput bytes/Sec    TX %3.3f (raw %3.3f) Gb/s - "
            "packets/Sec  TX %3.3f Kpps\n",
            tx_th * 8 / AXNP_RES_BYTE_SCALE, tx_raw_th * 8 / AXNP_RES_BYTE_SCALE,
            tx_pps / AXNP_RES_PKT_SCALE);

    printf("Wainting RX checks...\n");

    /* receive elapsed rx throughput time form server_id */
    elapsed_rx_nsec = 0;
    pld_recv_size = sizeof(payload);
    err =  axiom_recv_raw(s->dev, &src_node, &port, &type, &pld_recv_size,
            &payload);
    if (!AXIOM_RET_IS_OK(err) || (src_node != s->server_id) ||
            payload.command != AXIOM_CMD_NETPERF_END) {
        EPRINTF("recv_elapsed_time error - err: 0x%x node: 0x%x [0x%x] "
                "command 0x%x [0x%x]", err, src_node, s->server_id,
                payload.command, AXIOM_CMD_NETPERF_END);
        return -1;
    }

    elapsed_rx_nsec = payload.elapsed_time;

    rx_th = (double)(s->client.sent_bytes) / nsec2sec(elapsed_rx_nsec);
    rx_raw_th = (double)(s->client.sent_raw_bytes) / nsec2sec(elapsed_rx_nsec);
    rx_pps = (double)(s->client.total_packets) / nsec2sec(elapsed_rx_nsec);

    IPRINTF(verbose, "elapsed_tx_nsec = %" PRIu64 " - elapsed_rx_nsec = %"
            PRIu64, elapsed_nsec, elapsed_rx_nsec);

    printf("Throughput bytes/Sec    RX %3.3f (raw %3.3f) Gb/s - "
            "packets/Sec  RX %3.3f Kpps\n",
            rx_th * 8 / AXNP_RES_BYTE_SCALE, rx_raw_th * 8 / AXNP_RES_BYTE_SCALE,
            rx_pps / AXNP_RES_PKT_SCALE);

    if (payload.error) {
        printf("\n Remote node reports some ERRORS [%u]\n", payload.error);
    }

    return 0;
}

axiom_err_t
axnetperf_client(axnetperf_status_t *s)
{
    int ret = 0;

    /* generate random magic */
    srand(time(NULL));
    s->client.magic = rand() % 255;

    /* init subsystems */
    switch (s->np_type) {
        case AXNP_RDMA:
            ret = axnetperf_rdma_init(s);
            break;

        case AXNP_RAW:
            ret = axnetperf_raw_init(s);
            break;

        case AXNP_LONG:
            ret = axnetperf_long_init(s);
            break;

        default:
            EPRINTF("axiom-netperf type invalid");
            ret = -1;
    }

    if (ret) {
        EPRINTF("init failed");
        return ret;
    }

    /* send start message */
    ret = axnetperf_start(s);
    if (ret) {
        return ret;
    }

    switch (s->np_type) {
        case AXNP_RDMA:
            ret = axnetperf_rdma(s);
            break;

        case AXNP_RAW:
        case AXNP_LONG:
            ret = axnetperf_raw_long(s);
            break;

        default:
            EPRINTF("axiom-netperf type invalid");
            return ret;
    }

    /* receive end message */
    ret = axnetperf_stop(s);
    if (ret) {
        EPRINTF("axiom-netperf stop failed");
    }

    return ret;
}
