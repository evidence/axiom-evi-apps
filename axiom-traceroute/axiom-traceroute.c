/*!
 * \file axiom-traceroute.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-traceroute application.
 *
 * axiom-traceroute prints the hops needed to reach the specified axiom node
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

#include <sys/types.h>
#include <sys/time.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-traceroute [arguments] -d dest_node \n");
    printf("AXIOM traceroute: print the hops needed to reach the specified dest_node\n\n");
    printf("Arguments:\n");
    printf("-d, --dest     dest_node   destination node of traceroute\n");
    printf("-v, --verbose              verbose output\n");
    printf("-h, --help                 print this help\n\n");
}

static int
recv_tracereoute_reply(axiom_dev_t *dev, axiom_node_id_t *recv_node,
                       axiom_port_t *port, axiom_type_t *type,
                       axiom_traceroute_payload_t *recv_payload)
{
    axiom_msg_id_t msg_err;
    axiom_payload_size_t payload_size = sizeof(*recv_payload);

    msg_err =  axiom_recv_raw(dev, recv_node, port, type, &payload_size,
            recv_payload);

    if (msg_err != AXIOM_RET_OK) {
        EPRINTF("receive error");
        return -1;
    }
    if (recv_payload->command != AXIOM_CMD_TRACEROUTE_REPLY) {
        EPRINTF("command received [%x] != AXIOM_CMD_TRACEROUTE_REPLY [%x]",
                recv_payload->command, AXIOM_CMD_TRACEROUTE_REPLY);
        return -1;
    }

    return 0;
}


int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_node_id_t dest_node, recv_node = 0;
    axiom_if_id_t if_id;
    axiom_err_t err;
    axiom_msg_id_t msg_err, node_id;
    axiom_port_t port;
    axiom_type_t type;
    axiom_traceroute_payload_t payload, recv_payload;
    uint8_t received_reply = 0, expected_reply = 0;
    int dest_node_ok = 0;
    int long_index =0;
    int opt = 0;
    int recv_err;
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
                    EPRINTF("wrong number of nodes");
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
        exit(-1);
    }

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    /* bind the current process on port */
    err = axiom_bind(dev, AXIOM_RAW_PORT_NETUTILS);
    if (err != AXIOM_RET_OK) {
        EPRINTF("axiom_bind error");
        exit(-1);
    }

    /* get interface to reach next hop for dest_node */
    err = axiom_next_hop(dev, dest_node, &if_id);
    if (err != AXIOM_RET_OK) {
        EPRINTF("node[%u] is unreachable", dest_node);
        exit(-1);
    }

    printf("Node %u, start traceroute to node %u, %d hops max\n", node_id,
            dest_node, AXIOM_NODES_MAX);

    type = AXIOM_TYPE_RAW_NEIGHBOUR;
    port = AXIOM_RAW_PORT_INIT;
    payload.command = AXIOM_CMD_TRACEROUTE;
    payload.src_id = node_id;
    payload.dst_id = dest_node;
    payload.step = 0;

    /* send initial raw neighbour traceroute message */
    msg_err = axiom_send_raw(dev, if_id, port, type, sizeof(payload),
            &payload);

    if (msg_err != AXIOM_RET_OK) {
        EPRINTF("send error");
        goto err;
    }

    expected_reply = AXIOM_NODES_MAX;
    do {
        recv_err = recv_tracereoute_reply(dev, &recv_node, &port,
                &type, &recv_payload);
        if (recv_err == -1) {
            goto err;
        }
        received_reply++;
        printf("%u  -  node %u\n", recv_payload.step, recv_node);

        /* last node reply contains the number of steps */
        if (recv_node == dest_node) {
            expected_reply = recv_payload.step;
        }

    } while (received_reply < expected_reply);

    printf("\n--- %u hops to reach node %u ---\n", received_reply, dest_node);

err:
    axiom_close(dev);

    return 0;

}
