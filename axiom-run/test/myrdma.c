/*!
 * \file myrdma.c
 *
 * \version     v1.2
 *
 * A simple program to test axiom rdma/long message exchange
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
#include "axiom_allocator.h"

#include "axiom_run_api.h"

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
#include <string.h>

#define DEBUG

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

static axiom_dev_t *dev;
static int mynode,yournode;

#define STARTCOUNTER 7
#define BREAKCOUNTER 9421

#define BTYPE uint16_t
//#define MASK 0xffff

static inline unsigned long next_c(unsigned long c) {
    while (c>BREAKCOUNTER) c=c-BREAKCOUNTER+STARTCOUNTER;
    return c;
}

// number of transfert
#define NUM 40
// size of every transfer
#define RDMASIZE (98000*sizeof(BTYPE))
// rdma memory required
#define SIZENEED (NUM*RDMASIZE*2*2)

#define PORT 3

BTYPE *source;

BTYPE *end;

void  *sender(void *data) {
    unsigned long c;
    int i,j,ret;
    BTYPE *source_addr;

#ifdef DEBUG
    fprintf(stderr,"sender: start\n");
#endif
   
    usleep(250000);
    c=STARTCOUNTER;
    for (i=0;i<NUM-1;i++) {
        source_addr=source;
        for (j=0;j<RDMASIZE/sizeof(BTYPE);j++) {

            if (source>=end) {
                fprintf(stderr,"ERROR ERROR ERROR TRANS %p >= %p\n",source,end);
                sleep(1);
                exit(0);
            }

            *source=(BTYPE)c;
            source++;
            c++;
            if (c>BREAKCOUNTER) c=STARTCOUNTER;
        }
        fprintf(stderr,"s=%p\n",source_addr);
        ret=axiom_rdma_write(dev, yournode, RDMASIZE, source_addr, source_addr, NULL);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_rdma_write()");
        }
        ret=axiom_send_raw(dev,yournode, PORT, AXIOM_TYPE_RAW_DATA, sizeof(source_addr), &source_addr);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_send_raw()");
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
    axiom_raw_payload_size_t size;
    axiom_node_id_t src_id;
    axiom_type_t type;
    BTYPE *addr;
    int i,j,cm,err;
    unsigned long c;

#ifdef DEBUG
    fprintf(stderr,"receiver: start\n");
#endif

    err=cm=0;
    c=STARTCOUNTER;
    for (i=0;i<NUM;i++) {
        size=sizeof(addr);
        port=PORT;
        id= axiom_recv_raw(dev, &src_id, &port, &type, &size, (void*)&addr);
        fprintf(stderr,"received seq=%d %p expected %lu..%lu\n",cm++,addr,c,next_c(c+RDMASIZE/sizeof(BTYPE)-1));
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_raw");
            continue;
        }
        for (j=0;j<RDMASIZE/sizeof(BTYPE);j++) {

            if (addr>=end) {
                fprintf(stderr,"ERROR ERROR ERROR RRECV %p >= %p\n",addr,end);
                sleep(1);
                exit(0);
            }

            if (*addr!=(BTYPE)c) {
                fprintf(stderr,"ERROR expected %lu found %lu\n",c,(unsigned long)*addr);
                err++;
            }
            addr++;
            c++;
            if (c>BREAKCOUNTER) c=STARTCOUNTER;
        }

    }
    fprintf(stderr,"errors=%d\n",err);

#ifdef DEBUG
    fprintf(stderr,"receiver: end\n");
#endif

    return NULL;
}

int main(int argc, char**argv) {
    int opt,long_index;
    axiom_err_t err;
    pthread_t threcv, thsend;
    int res,ret;
    size_t privatesize=SIZENEED+32;
    size_t sharedsize=0;
    BTYPE *base;
    
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'h':
                fprintf(stderr, "usage: myrdma\n");
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

    ret = axiom_allocator_init(&privatesize, &sharedsize, AXAL_SW);
    if (ret) {
        perror("axiom_allocator_init()");
        exit(EXIT_FAILURE);
    }
    // hp: all nodes receive the same base address!
    base=axiom_private_malloc(SIZENEED);
    if (base==NULL) {
        perror("axiom_private_malloc()");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr,"allocated %lu bytes (%lu MiB) from %p to %p\n",(unsigned long)SIZENEED,(unsigned long)SIZENEED/1024/1024,base,base+SIZENEED/sizeof(BTYPE));
    end=(BTYPE*)(((uint8_t*)base)+SIZENEED);

    memset(base,0,SIZENEED);
    ret=axrun_sync(7, 0);
    if (ret) {
        perror("axrun_sync()");
        exit(EXIT_FAILURE);
    }

    source=base;
    if (mynode&0x1) source+=SIZENEED/2/sizeof(BTYPE);
    
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
