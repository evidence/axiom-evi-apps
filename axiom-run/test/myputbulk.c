/*!
 * \file myputbulk.c
 *
 * \version     v0.12
 *
 * A simple program to test axiom long message exchange
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <sys/types.h>
#include <stdint.h>

#include "axiom_nic_types.h"
#include "axiom_nic_limits.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_regs.h"
#include "axiom_nic_packets.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

static axiom_dev_t *dev;
static int mynode,yournode;

#define NUM 512
#define EVERY 16

#define PORT 3

void send(void) {
    axiom_msg_id_t id;
    uint16_t buf[2];
    int i;
    
    for (i=0;i<NUM;i++) {
        buf[0]=mynode;
        buf[1]=i;
        id=axiom_send_long(dev, yournode, PORT, sizeof(buf), buf);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_send_long");
        }
        if (i%EVERY==0) printf("SENT %i\n",i);
    }
}

void recv(void) {
    axiom_msg_id_t id;
    axiom_port_t port;
    axiom_long_payload_size_t size;
    axiom_node_id_t src_id;
    uint16_t buf[2];
    int i;
    
    for (i=0;i<NUM;i++) {
        size=sizeof(buf);
        port=PORT;
        id= axiom_recv_long(dev, &src_id, &port, &size, buf);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_long");
            continue;
        }
        if (i%EVERY==0) printf("RECV %i\n",i);
    }
}

void reply(void) {
    axiom_msg_id_t id;
    axiom_port_t port;
    axiom_long_payload_size_t size;
    axiom_node_id_t src_id;
    uint16_t buf[2];
    int i;

    for (i=0;i<NUM;i++) {
        size=sizeof(buf);
        port=PORT;
        id= axiom_recv_long(dev, &src_id, &port, &size, buf);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_long");
            continue;
        }
        if (i%EVERY==0) printf("RECV FOR REP %i\n",i);
        id=axiom_send_long(dev, yournode, PORT, sizeof(buf), buf);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_send_long");
        }
        if (i%EVERY==0) printf("SENT FOR REP %i\n",i);
    }
}

int main(int argc, char**argv) {
    int opt,long_index;
    axiom_err_t err;
    
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, "usage: myputbulk\n");
                exit(0);
            case '?':
                break;
        }

    }
    
    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(EXIT_FAILURE);
    }

    err = axiom_bind(dev, 3);
    if (!AXIOM_RET_IS_OK(err)) {
        perror("axiom_bind()");
        exit(EXIT_FAILURE);
    }
    
    mynode=(int) axiom_get_node_id(dev);
    yournode=((mynode-1)^0x01)+1; // now the node id start fromONE!
    
    if ((mynode&1)==0) {
        printf("STARTING SENDER\n");
        send();
        recv();
    } else {
        printf("STARTING REPLAYER\n");
        reply();
    }

    axiom_close(dev);    
    return EXIT_SUCCESS;
}
