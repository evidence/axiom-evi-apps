/*!
 * \file myexch.c
 *
 * \version     v0.10
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
#include <stdint.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#define DEBUG

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

static axiom_dev_t *dev;
static int mynode,yournode;

#define NUM 4096

#define PORT 3

void  *sender(void *data) {
    axiom_msg_id_t id;
    uint16_t buf[2];
    int i;

#ifdef DEBUG
    fprintf(stderr,"sender: start\n");
#endif
    
    usleep(250000);
    for (i=0;i<NUM;i++) {
        buf[0]=mynode;
        buf[1]=i;
        id=axiom_send_long(dev, yournode, PORT, sizeof(buf), buf);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_send_long");
        }        
    }

#ifdef DEBUG
    fprintf(stderr,"sender: end\n");
#endif
    
    return NULL;    
}

void  *receiver(void *data) {
    axiom_msg_id_t id;
    axiom_port_t port;
    axiom_long_payload_size_t size;
    axiom_node_id_t src_id;
    uint16_t buf[2];
    int i,c;

#ifdef DEBUG
    fprintf(stderr,"receiver: start\n");
#endif
    
    for (c=0, i=0;i<NUM;i++, c++) {
        size=sizeof(buf);
        port=PORT;
        id= axiom_recv_long(dev, &src_id, &port, &size, buf);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_long");
            continue;
        }
        fprintf(stderr,"src_node=0x%02x msgid=%03d seq=%04d %s %s\n",buf[0],id,buf[1],buf[1]!=c?"OUT OF SEQ":"",buf[1]<c?"DUP":"");
        c=buf[1];
        if (src_id!=yournode) {
            fprintf(stderr,"source id mismatch!\n");
        }
        if (buf[0]!=yournode) {
            fprintf(stderr,"buffer source byte mismatch!\n");
        }
    }

#ifdef DEBUG
    fprintf(stderr,"receiver: end\n");
#endif

    return NULL;
}

int main(int argc, char**argv) {
    int opt,long_index;
    axiom_err_t err;
    pthread_t threcv, thsend;
    int res;

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, "usage: myexch\n");
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
    yournode=((mynode-1)^0x01)+1; // now the node number start from ONE!
    
    res = pthread_create(&threcv, NULL, receiver, NULL);
    if (res != 0) {
        perror("pthread_create()");
        exit(EXIT_FAILURE);
    }

    res = pthread_create(&thsend, NULL, sender, NULL);
    if (res != 0) {
        perror("pthread_create()");
        exit(EXIT_FAILURE);
    }

#ifdef DEBUG
    fprintf(stderr, "waiting...\n");
#endif

    res = pthread_join(thsend, NULL);
    if (res != 0) {
        perror("pthread_join()");
        exit(EXIT_FAILURE);
    }
    
    res = pthread_join(threcv, NULL);
    if (res != 0) {
        perror("pthread_join()");
        exit(EXIT_FAILURE);
    }
            
    axiom_close(dev);    
    return EXIT_SUCCESS;
}
