
/*
 *
 * A simple program to test axiom rdma/long message exchange
 *
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

#define STARTCOUNTER 0
#define BREAKCOUNTER 9421

#define BTYPE uint16_t
#define MASK 0xffff

// number of transfert
#define NUM 4096
// size of evry transfer
#define RDMASIZE (512*sizeof(BTYPE))
// rdma memory required
#define SIZENEED (NUM*RDMASIZE*2)

#define PORT 3

BTYPE *source;

void  *sender(void *data) {
    unsigned long c;
    int i,j,ret;
    BTYPE *source_addr;

#ifdef DEBUG
    fprintf(stderr,"sender: start\n");
#endif
    
    usleep(250000);
    c=STARTCOUNTER;
    for (i=0;i<NUM;i++) {
        source_addr=source;
        for (j=0;j<RDMASIZE/sizeof(BTYPE);j++) {
            *source=c&MASK;
            source++;
            c++;
            if (c>BREAKCOUNTER) c=STARTCOUNTER;
        }
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
        fprintf(stderr,"received seq=%d %p expected %lu..%lu\n",cm++,addr,c&MASK,(c+RDMASIZE/sizeof(BTYPE)-1)&MASK);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_raw");
            continue;
        }
        for (j=0;j<RDMASIZE/sizeof(BTYPE);j++) {
            if (*addr!=(c&MASK)) {
                fprintf(stderr,"ERROR expected %lu found %lu\n",c&MASK,(unsigned long)*addr);
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

    memset(base,0,SIZENEED);
    ret=axrun_sync(7, 0);
    if (ret) {
        perror("axrun_sync()");
        exit(EXIT_FAILURE);
    }

    source=base;
    if (mynode&0x1) source+=SIZENEED/2;
    
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
