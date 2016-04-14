/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-23
 *
 * This file implementas the axiom-traceroute application
 *
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

static void usage(void)
{
    printf("usage: ./axiom-traceroute [[-n dest_node] | [-v] | [-h]] \n");
    printf("AXIOM traceroute\n\n");
    printf("-n,             dest_node   destination node of traceroute\n");
    printf("-v, --verbose               verbose output\n");
    printf("-h, --help                  print this help\n\n");
}

static int
recv_tracereoute_reply(axiom_dev_t *dev, axiom_node_id_t *recv_node,
                       axiom_port_t *port, axiom_flag_t *flag,
                       axiom_traceroute_payload_t *recv_payload)
{
    axiom_msg_id_t msg_err;

    msg_err =  axiom_recv_small(dev, recv_node, port, flag, (axiom_payload_t*)recv_payload);
    if (msg_err == AXIOM_RET_ERROR)
    {
        EPRINTF("receive error");
        return -1;
    }
    if (*flag == AXIOM_SMALL_FLAG_NEIGHBOUR)
    {
        EPRINTF("receive AXIOM_SMALL_FLAG_NEIGHBOUR message");
        return -1;
    }
    if (*port != AXIOM_SMALL_PORT_INIT)
    {
        EPRINTF("port not equal to AXIOM_SMALL_PORT_INIT");
        return -1;
    }
    if (recv_payload->command != AXIOM_CMD_TRACEROUTE_REPLY)
    {
        EPRINTF("message not equal to AXIOM_CMD_TRACEROUTE_REPLY");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_node_id_t dest_node, recv_node = 0;
    axiom_if_id_t my_if;
    axiom_err_t err;
    axiom_msg_id_t msg_err, my_node_id;
    axiom_port_t port;
    axiom_flag_t flag;
    axiom_traceroute_payload_t payload, recv_payload;
    uint8_t received_reply = 0, expected_reply = 0;
    int dest_node_ok = 0;
    int long_index =0;
    int opt = 0;
    int recv_err;
    static struct option long_options[] = {
        {"n", required_argument, 0, 'n'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv,"vhp:d:i:c:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'n' :
                if (sscanf(optarg, "%" SCNu8, &dest_node) != 1)
                {
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
    if (dest_node_ok == 1)
    {
        my_node_id = axiom_get_node_id(dev);

        /* bind the current process on port */
#if 0
        if (port_ok == 1) {
            err = axiom_bind(dev, port);
        }
#endif

        /* get interface to reach next hop for dest_node */
        err = axiom_next_hop(dev, dest_node, &my_if);
        if (err == AXIOM_RET_ERROR)
        {
            EPRINTF("axiom_next_hop error");
            exit(-1);
        }

        printf("Node %u, start traceroute of node = %u\n\n",
               my_node_id, dest_node);

        flag = AXIOM_SMALL_FLAG_NEIGHBOUR;
        port = AXIOM_SMALL_PORT_INIT;
        payload.command = AXIOM_CMD_TRACEROUTE;
        payload.src_id = my_node_id;
        payload.dst_id = dest_node;
        payload.step = 0;
        /* get small neighbour traceroute message */
        msg_err = axiom_send_small(dev, my_if, port, flag,
                                   (axiom_payload_t *)&payload);

        if (msg_err == AXIOM_RET_ERROR)
        {
            EPRINTF("send error");
            goto err;
        }
        memset (&recv_payload, 0, sizeof(recv_payload));
        do
        {
            recv_err = recv_tracereoute_reply(dev, &recv_node, &port,
                                              &flag, &recv_payload);
            if (recv_err == -1)
            {
                goto err;
            }
            received_reply++;
            printf("[hop %u] \n", recv_payload.step);
            printf("\t- node_id = %u\n\n", recv_node);

            /* wait for traceroute destination node reply*/
        } while (recv_node != dest_node);

        if (received_reply == recv_payload.step)
        {
            /* the number of steps present into the traceroute
             * destination node reply is equal to the number of
             * the already received messages: end of traceroute */
            printf("End traceroute\n");
        }
        else
        {
            /* I have to receive other messages */
            expected_reply = recv_payload.step;
            while (received_reply != expected_reply)
            {
                recv_err = recv_tracereoute_reply(dev, &recv_node, &port,
                                                  &flag, &recv_payload);
                if (recv_err == -1)
                {
                    goto err;
                }
                received_reply++;
                printf("[hop %u] \n", recv_payload.step);
                printf("\t- node_id = %u\n\n", recv_node);
            }
            printf("End traceroute\n");
        }
    }
    else
    {
        usage();
        exit(-1);
    }

err:
    axiom_close(dev);

    return 0;

}











#if 0
int verbose = 0;

static volatile int sigint_received = 0;

static void usage(void)
{
    printf("usage: ./axiom-traceroute [[-p port] | [-i interval] | [-c count] | \n");
    printf("                                         [-v] | [-h]] -d dest \n");
    printf("AXIOM ping\n\n");
    printf("-p, --port      port     port used for sending\n");
    printf("-d, --dest      dest     dest node id \n");
    printf("-i, --interval  interval ms between two ping messagges \n");
    printf("-c, --count     count    number of ping messagges to send \n");
    printf("-v, --verbose            verbose output\n");
    printf("-h, --help               print this help\n\n");
}

/* control-C handler */
static void sigint_h(int sig)
{
    sigint_received = 1;
}

/* TODO: allow the user to change the resolution */
static double usec2msec(uint64_t usec)
{
    return ((double)(usec) / 1000);
}


int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_msg_id_t send_ret, recv_ret, my_node_id;
    axiom_port_t port = 1, recv_port ;
    axiom_node_id_t dst_id, src_id;
    axiom_flag_t flag = 0;
    axiom_ping_payload_t payload, recv_payload;
    unsigned int interval = 500; /* default interval 500 ms */
    unsigned int num_ping = 1;
    int port_ok = 0, dst_ok = 0, num_ping_ok = 0;
    struct timeval start_tv, end_tv, diff_tv;
    uint64_t diff_usec, sum_usec = 0, max_usec = 0, min_usec = -1;
    double avg_usec;
    int sent_packets = 0, recv_packets = 0;
    int ret;

    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"dest", required_argument, 0, 'd'},
        {"port", required_argument, 0, 'p'},
        {"interval", required_argument, 0, 'i'},
        {"count", required_argument, 0, 'c'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    signal(SIGINT, sigint_h);

    while ((opt = getopt_long(argc, argv,"vhp:d:i:c:",
                         long_options, &long_index )) != -1)
    {
        switch (opt)
        {
            case 'p' :
                if (sscanf(optarg, "%" SCNu8, &port) != 1)
                {
                    EPRINTF("wrong port");
                    usage();
                    exit(-1);
                }
                port_ok = 1;
                break;

            case 'd' :
                if (sscanf(optarg, "%" SCNu8, &dst_id) != 1)
                {
                    EPRINTF("wrong destination");
                    usage();
                    exit(-1);
                }
                dst_ok = 1;
                break;

            case 'i':
                if (sscanf(optarg, "%" SCNu32, &interval) != 1)
                {
                    EPRINTF("wrong interval");
                    usage();
                    exit(-1);
                }
                break;

            case 'c':
                if (sscanf(optarg, "%" SCNu32, &num_ping) != 1)
                {
                    EPRINTF("wrong count");
                    usage();
                    exit(-1);
                }
                num_ping_ok = 1;
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

    /* check port parameter */
    if (port_ok == 1)
    {
        /* port parameter inserted */
        if ((port <= 0) || (port > 255))
        {
            printf("Port not allowed [%u]; [0 < port < 256]\n", port);
            exit(-1);
        }
    }

    /* check destination node parameter */
    if (dst_ok == 1)
    {
        /* destination node inserted */
        if ((dst_id < 0) || (dst_id > 254))
        {
            printf("Destination node id not allowed [%u]; [0 <= dst_id < 255]\n", dst_id);
            exit(-1);
        }
    }
    else
    {
        usage();
        exit(-1);
    }
#ifndef AXIOM_NO_TX
    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    my_node_id = axiom_get_node_id(dev);
#endif
    /* bind the current process on port */
#if 0
    if (port_ok == 1) {
        err = axiom_bind(dev, port);
    }
#endif
    printf("PING node %d.\n", dst_id);

    payload.packet_id = 0;
    while (!sigint_received &&  (num_ping > 0))
    {
        IPRINTF(verbose,"[node %u] sending ping message...\n", my_node_id);
#ifndef AXIOM_NO_TX
        /* send a small message*/
        payload.command = AXIOM_PING;
        payload.packet_id = (uint16_t)(payload.packet_id + 1);
        send_ret =  axiom_send_small(dev, (axiom_node_id_t)dst_id,
                                            (axiom_port_t)port, flag,
                                            (axiom_payload_t*)&payload);
        if (send_ret == AXIOM_RET_ERROR)
        {
               EPRINTF("send error");
               goto err;
        }
#endif
        /* get actual time */
        ret = gettimeofday(&start_tv,NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        IPRINTF(verbose,"timestamp: %ld sec\t%ld microsec\n", start_tv.tv_sec,
                                                              start_tv.tv_usec);
        sent_packets++;
#ifndef AXIOM_NO_TX
        IPRINTF(verbose,"[node %u] message sent to port %u\n", my_node_id, port);
        IPRINTF(verbose,"\t- destination_node_id = %u\n", dst_id);

        /* ********************* receive the reply ************************ */
        /* receive a small message from port*/
        recv_ret =  axiom_recv_small(dev, &src_id, (axiom_port_t *)&recv_port,
                                     &flag, (axiom_payload_t*)&recv_payload);
        if (recv_ret == AXIOM_RET_ERROR)
        {
            EPRINTF("receive error");
            break;
        }
#else
        usleep(1234000);
#endif

        if (recv_payload.command != AXIOM_PONG)
        {
            EPRINTF("receive a no AXIOM-PONG message");
            continue;
        }
        recv_packets++;
        /* get actual time */
        ret = gettimeofday(&end_tv,NULL);
        if (ret == -1)
        {
            EPRINTF("gettimeofday error");
            goto err;
        }
        IPRINTF(verbose,"[node %u] reply received on port %u\n", my_node_id, recv_port);
        IPRINTF(verbose,"\t- source_node_id = %u\n", src_id);
        IPRINTF(verbose,"\t- message index = %u\n", recv_payload.packet_id);
        IPRINTF(verbose,"timestamp: %ld sec\t%ld microsec\n", end_tv.tv_sec,
                                                   end_tv.tv_usec);
        /* TODO: check serial number? */

        timersub(&end_tv, &start_tv, &diff_tv);

        /* ********************** statiscs ***************************** */
        /* difference computation */
        diff_usec = diff_tv.tv_usec + (diff_tv.tv_sec * 1000000);
        sum_usec += diff_usec;

        printf("from node %d: seq_num=%d time=%3.3f ms\n", src_id,
                recv_payload.packet_id,
                usec2msec(diff_usec));

        if (diff_usec > max_usec)
        {
            max_usec = diff_usec;
        }
        if (diff_usec < min_usec)
        {
            min_usec = diff_usec;
        }

        usleep(interval * 1000);

        if (num_ping_ok == 1)
        {
            num_ping--;
        }

    }

    /* average computation */
    avg_usec = ((double)(sum_usec) / recv_packets);

    printf("\n--- node %d ping statistics ---\n", dst_id);
    printf("%d packets transmitted, %d received, %d packet loss\n",
           sent_packets, recv_packets, recv_packets - sent_packets);
    printf("rtt min/avg/max = %3.3f/%3.3f/%3.3f ms\n", usec2msec(min_usec),
            usec2msec(avg_usec), usec2msec(max_usec));

err:
    axiom_close(dev);


    return 0;
}
#endif
