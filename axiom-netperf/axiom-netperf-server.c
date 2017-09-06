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
#include <pthread.h>
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

void
axiom_netperf_send_reply(axiom_dev_t *dev, axnetperf_server_t *server,
        axiom_node_id_t src, uint8_t error_report, int verbose)
{
    axiom_err_t err;
    axiom_netperf_payload_t payload;
    struct timespec elapsed_ts;
    uint64_t elapsed_nsec;
    double rx_th;

    IPRINTF(verbose,"End timestamp: %ld sec %ld nanosec",
            server->cur_ts.tv_sec, server->cur_ts.tv_nsec);

    /* compute time elapsed */
    elapsed_ts = timespec_sub(server->cur_ts, server->start_ts);
    elapsed_nsec = timespec2nsec(elapsed_ts);
    rx_th = (double)(server->received_bytes / nsec2sec(elapsed_nsec));
    IPRINTF(verbose, "Rx throughput = %3.3f Gb/s - elapsed_nsec = %llu",
            rx_th * 8 / 1024 / 1024 / 1024, (long long unsigned)elapsed_nsec);

    /* send elapsed time to netperf application */
    payload.command = AXIOM_CMD_NETPERF_END;
    payload.total_bytes = server->received_bytes;
    payload.elapsed_time = elapsed_nsec;
    payload.error = error_report;
    err = axiom_send_raw(dev, src, server->reply_port,
            AXIOM_TYPE_RAW_DATA, sizeof(payload), &payload);
    if (!AXIOM_RET_IS_OK(err)) {
        EPRINTF("send back time error");
        return;
    }

    printf("Session with node %d ended\n", src);
}


static void
axiom_netperf_reply(axnetperf_status_t *s, axiom_node_id_t src, size_t payload_size,
        void *payload, int verbose)
{
    axiom_netperf_payload_t *recv_payload =
            ((axiom_netperf_payload_t *) payload);
    axnetperf_server_t *server = &s->server[src];

    /* take a timestamp */
    if (clock_gettime(CLOCK_REALTIME, &server->cur_ts)) {
        EPRINTF("gettime error");
        return;
    }

    if (recv_payload->command == AXIOM_CMD_NETPERF_START) {

        printf("Session with node %d started\n", src);

        /* TODO: define a macro */
        server->expected_bytes = recv_payload->total_bytes;
        /* reset start time */
        //memset(&server->start_ts, 0, sizeof(server->start_ts));
        server->received_bytes = 0;
        server->reply_port = recv_payload->reply_port;

        /* get time of the first netperf message received */
        memcpy(&server->start_ts, &server->cur_ts, sizeof(server->start_ts));
        IPRINTF(verbose,"Start timestamp: %ld sec %ld nsec - Reply-port: 0x%x",
                server->cur_ts.tv_sec, server->cur_ts.tv_nsec,
                server->reply_port);
        return;
    } else if (recv_payload->command == AXIOM_CMD_NETPERF_END) {
        int i;
        uint8_t failed = 0;
        void *rdma_zone;
        uint64_t rdma_size;

        /* check RDMA */
        if (recv_payload->type != AXNP_RDMA)
            return;

        /* RDMA end message */
        server->received_bytes = recv_payload->total_bytes;

        /* map RDMA zone */
        rdma_zone = axiom_rdma_mmap(s->dev, &rdma_size);
        if (!rdma_zone) {
            EPRINTF("rdma map failed");
            failed = 1;
        }

        if (!failed) {
            /* TODO: check magic */
            for (i = 0; i < server->received_bytes; i += 8) {
                if (*(uint64_t *)(((uint8_t *)rdma_zone) + i) !=
                        recv_payload->magic) {
                    failed = 1;
                    break;
                }
            }
            axiom_rdma_munmap(s->dev);
        }

        axiom_netperf_send_reply(s->dev, server, src, failed, verbose);
    } else if (recv_payload->command == AXIOM_CMD_NETPERF) {
        /* RAW or LONG message */
        server->received_bytes += payload_size;

        DPRINTF("NETPERF msg received from: %u - expected_bytes: %llu "
                "received_bytes: %llu", src,
                (long long unsigned)server->expected_bytes,
                (long long unsigned)server->received_bytes);

        if (server->received_bytes >= server->expected_bytes) {
            axiom_netperf_send_reply(s->dev, server, src, 0, verbose);
        }
    } else {
        EPRINTF("receive a not AXIOM_CMD_NETPERF message");
        return;
    }
}

void *
axnetperf_server(void *arg)
{
    axnetperf_status_t *s = ((axnetperf_status_t *) arg);
    axiom_node_id_t src;
    axiom_port_t port;
    axiom_type_t type;
    axiom_long_payload_t payload;
    axiom_err_t err;

    int run = 1;

    printf("[TID %ld] SERVER started on port %d\n", gettid(), s->server_port);

    while(run) {
        size_t payload_size = sizeof(payload);

        err = axiom_recv(s->dev, &src, &port, &type, &payload_size, &payload);
        if (!AXIOM_RET_IS_OK(err)) {
            EPRINTF("error receiving message");
            break;
        }

        pthread_mutex_lock(&s->mutex);
        axiom_netperf_reply(s, src, payload_size, &payload, verbose);
        pthread_mutex_unlock(&s->mutex);
    }

    return AXIOM_RET_OK;
}
