/*!
 * \file testrdma.c
 *
 * \version     v0.12
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

#include "../common/test.h"

static int debug=0;

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"seed", required_argument, 0, 's'},
    {"num", required_argument, 0, 'n'},
    {"block", required_argument, 0, 'b'},
    {"guard", required_argument, 0, 'g'},
    {0, 0, 0, 0}
};

// seed for rand
#define SEED 42
// used axiom port to send raw messages
#define PORT 4
// number of block to transfert
#define NUM 64
// size of every transfer
#define BUFSIZE 32768
// size of guard block between rdma blocks
#define GUARD 128
// rdma memory required
#define SIZENEED ((num*(blocksize+guard)+guard)*2)
//#define SIZENEED 269488128

static axiom_dev_t *dev;
static int mynode,yournode;
static int port=PORT;

/*
#define MEMBLOCKSTR "%p...%p"
#define MEMBLOCKDATA(addr,size) ((uint8_t*)(addr)),((uint8_t*)(addr))+((size_t)(size))-1

#define MEMBLOCKFULLSTR "%p...%p %lu 0x%lx (%lu KiB %lu MiB)"
#define MEMBLOCKFULLDATA(addr,size) ((uint8_t*)(addr)),((uint8_t*)(addr))+((size_t)(size))-1,((size_t)(size)),((size_t)(size)),((size_t)(size))/1024,((size_t)(size))/1024/1024

#define SIZESTR "%lu 0x%lx (%lu KiB %lu MiB)"
#define SIZEDATA(size) ((size_t)(size)),((size_t)(size)),((size_t)(size))/1024,((size_t)(size))/1024/1024
*/

/*
static int rand_counter_r=0,rand_value_r=0;
static unsigned int rand_state_r;
static inline uint8_t next_rand_r() {
    return rand_counter_r--==0?(rand_counter_r=sizeof(int)-1,rand_value_r=rand_r(&rand_state_r),rand_value_r&0xff):(rand_value_r>>=1,rand_value_r&0xff);
}

static int rand_counter_t=0,rand_value_t=0;
static unsigned int rand_state_t;
static inline uint8_t next_rand_t() {
    return rand_counter_t--==0?(rand_counter_t=sizeof(int)-1,rand_value_t=rand_r(&rand_state_t),rand_value_t&0xff):(rand_value_t>>=1,rand_value_t&0xff);
}
*/

static uint8_t *source;
static int num,blocksize,guard;
static unsigned seed;

void  *sender(void *data) {
    int i,j,ret;
    uint8_t *source_addr;
    rand_t r;

    if (debug>0)
        fprintf(stderr,"sender: start\n");
    
    usleep(250000);
    rand_init(&r,seed+mynode);
    for (i=0;i<num;i++) {
        source+=guard;
        source_addr=source;
        for (j=0;j<blocksize;j++) {
            *source=rand_next(&r);
            source++;
        }

        if (debug>1)
            fprintf(stderr,"sending  " MEMBLOCKSTR "\n", MEMBLOCKDATA(source_addr,blocksize));
        if (debug>2)
            dump(stderr,source_addr,blocksize);

        ret=axiom_rdma_write_sync(dev, yournode, blocksize, source_addr, source_addr, NULL);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_rdma_write()");
        }
        ret=axiom_send_raw(dev,yournode, port, AXIOM_TYPE_RAW_DATA, sizeof(source_addr), &source_addr);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_send_raw()");
        }
    }

    if (debug>0)
        fprintf(stderr,"sender: end\n");
    
    return NULL;    
}

static int errors=0;

void  *receiver(void *data) {
    axiom_msg_id_t id;
    axiom_port_t myport;
    axiom_raw_payload_size_t size;
    axiom_node_id_t src_id;
    axiom_type_t type;
    uint8_t *addr,*start=NULL;
    uint8_t c;
    int i,j;
    int emit=0;
    rand_t r;

    if (debug>0)
        fprintf(stderr,"receiver: start\n");

    rand_init(&r,seed+yournode);
    for (i=0;i<num;i++) {
        size=sizeof(addr);
        myport=port;
        id= axiom_recv_raw(dev, &src_id, &myport, &type, &size, (void*)&addr);
        if (start==NULL) start=addr-guard;
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_raw");
            continue;
        }
        if (size!=sizeof(uint8_t*)) {
            fprintf(stderr,"ERROR wrong raw message size %u (expected %ld)\n",(unsigned)size,sizeof(uint8_t*));
            errors++;
            continue;
        }

        if (debug>1)
            fprintf(stderr,"received " MEMBLOCKSTR "\n", MEMBLOCKDATA(addr,blocksize));
        if (debug>2)
            dump(stderr,addr,blocksize);

        // CHECK rdma block
        // Note that in case of raw message reordering the following test fail :-(
        emit=1;
        for (j=0;j<blocksize;j++) {
            c=rand_next(&r);
            if (*addr!=c) {
                errors++;
                if (emit) {
                    fprintf(stderr,"ERROR expected 0x%02x found 0x%02x\n",(unsigned)c,(unsigned)*addr);
                    emit=0;
                }
            }
            addr++;
        }
    }

    // FINAL CHECK all memory and block guard
    rand_init(&r,seed+yournode);
    addr=start;
    for (i=0;i<num;i++) {
        emit=1;
        for (j=0;j<guard;j++) {
            if (*addr!=0) {
                if (emit) {
                    fprintf(stderr,"ERROR guard block dirty, found 0x%02x\n",(unsigned)*addr);
                    emit=0;
                }
                errors++;
            }
            addr++;
        }
        emit=1;
        for (j=0;j<blocksize;j++) {
            c=rand_next(&r);
            if (*addr!=c) {
                errors++;
                if (emit) {
                    fprintf(stderr,"ERROR expected 0x%02x found 0x%02x\n",(unsigned)c,(unsigned)*addr);
                    emit=0;
                }
            }
            addr++;
        }
    }
    emit=1;
    for (j=0;j<guard;j++) {
        if (*addr!=0) {
            if (emit) {
                fprintf(stderr,"ERROR guard block dirty, found 0x%02x\n",(unsigned)*addr);
                emit=0;
            }
            errors++;
        }
        addr++;
    }


    fprintf(stderr,"errors=%d\n",errors);

    if (debug>0)
        fprintf(stderr,"receiver: end\n");

    return NULL;
}

/*
static inline int i_am_the_first(int mynode) {
    char *s;
    long int nodes;
    s= getenv("AXIOM_NODES");
    if (s == NULL) {
        return 0;
    }
    nodes = strtol(s, NULL, 0);
    if (nodes==0) return 0;
    return __builtin_ctz(nodes)+1==mynode;
}
*/

static void help() {
    fprintf(stderr, "usage: testrdma [options]\n");
    fprintf(stderr, "  -d|--debug     debug (every -d increase verbosiness, max 3)\n");
    fprintf(stderr, "  -s|--seed SEED seed for srand [default: %d]\n",SEED);
    fprintf(stderr, "  -p|--port PORT axiom port [default: %d]\n",PORT);
    fprintf(stderr, "  -n|--num NUM   numbers of block transmitted/received [default: %d]\n",NUM);
    fprintf(stderr, "  -b|--block SZ  block size in bytes [min:%ld default:%d max:%d]\n",sizeof(unsigned int)+1,BUFSIZE,AXIOM_RDMA_PAYLOAD_MAX_SIZE);
    fprintf(stderr, "  -g|--guard SZ  guard block size [default: %d]\n",GUARD);
}

int main(int argc, char**argv) {
    size_t privatesize;

    int opt,long_index;
    axiom_err_t err;
    pthread_t threcv, thsend;
    int res,ret;
    size_t sharedsize=0;
    uint8_t*base;

    num=NUM;
    blocksize=BUFSIZE;
    guard=GUARD;
    
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hs:p:n:b:g:d", long_options, &long_index)) != -1) {
        switch (opt) {
             case 'd':
                debug++;
                break;
           case 's':
                seed=asc2int(optarg);
                break;
            case 'p':
                port=asc2int(optarg);
                break;
            case 'n':
                num=asc2int(optarg);
                break;
            case 'b':
                blocksize=asc2int(optarg);
                if (blocksize<sizeof(unsigned int)+1||blocksize>AXIOM_RDMA_PAYLOAD_MAX_SIZE) {
                    fprintf(stderr, "ERROR: bad block size (-b option)\n");
                    help();
                    exit(EXIT_FAILURE);
                }

                break;
            case 'g':
                guard=asc2int(optarg);
                break;
            case 'h':
                help();
                exit(EXIT_SUCCESS);
            case '?':
            default:
                fprintf(stderr, "ERROR: unknown option '%c'\n",opt);
                help();
                exit(EXIT_FAILURE);
        }
    }

    dev = axiom_open(NULL);
    if (dev == NULL) {
        perror("axiom_open()");
        exit(EXIT_FAILURE);
    }

    err = axiom_bind(dev, port);
    if (!AXIOM_RET_IS_OK(err)) {
        perror("axiom_bind()");
        exit(EXIT_FAILURE);
    }
    
    mynode=(int) axiom_get_node_id(dev);
    yournode=((mynode-1)^0x01)+1; // now the node number start from ONE!
    
    privatesize=SIZENEED+32;

    if (debug>0)
    fprintf(stderr,"axiom allocator requested " SIZESTR "\n", SIZEDATA(privatesize));
    
    ret = axiom_allocator_init(&privatesize, &sharedsize, AXAL_SW);
    if (ret) {
        perror("axiom_allocator_init()");
        exit(EXIT_FAILURE);
    }

    if (debug>0) {
        fprintf(stderr,"axiom allocator allocated " SIZESTR "\n", SIZEDATA(privatesize));
        fprintf(stderr,"axiom malloc size " SIZESTR "\n", SIZEDATA(SIZENEED));
    }

    // hp: all nodes receive the same base address!
    base=axiom_private_malloc(SIZENEED);
    if (base==NULL) {
        perror("axiom_private_malloc()");
        exit(EXIT_FAILURE);
    }

    if (debug>0) {
        fprintf(stderr,"axiom malloc " MEMBLOCKFULLSTR "\n", MEMBLOCKFULLDATA(base,SIZENEED));
        fprintf(stderr,"block size " SIZESTR "\n", SIZEDATA(blocksize));
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

    if (debug>0)
        fprintf(stderr, "waiting...\n");

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
    return errors==0?EXIT_SUCCESS:EXIT_FAILURE;
}
