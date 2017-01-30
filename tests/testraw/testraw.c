/*!
 * \file testraw.c
 *
 * \version     v0.10
 *
 * A simple program to test axiom raw (i.e. short) message exchange
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

int debug=0;

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"seed", required_argument, 0, 's'},
    {"num", required_argument, 0, 'n'},
    {"block", required_argument, 0, 'b'},
    {"debug", required_argument, 0, 'd'},
    {"full", no_argument, 0, 'F'},
    {"half", no_argument, 0, 'H'},
    {"multi", no_argument, 0, 'M'},
    {0, 0, 0, 0}
};

static axiom_dev_t *dev;
static int mynode,yournode,numnodes;

#define MODE_FULL 0
#define MODE_HALF 1
#define MODE_MULTI 2

// seed for rand
#define SEED 42
// used axiom port to send  messages
#define PORT 4
// number of block to transfert
#define NUM 64
// size of every transfer
#define BUFSIZE 64

typedef struct {
    int destnode;
} sender_data_t ;

static int num=NUM;
static int blocksize=BUFSIZE;
static int mode=MODE_FULL;
static int port=PORT;
static int seed=SEED;

void  *sender(void *data) {
    sender_data_t *p=(sender_data_t *)data;
    uint8_t buffer[AXIOM_RAW_PAYLOAD_MAX_SIZE];
    int i,j,ret;
    long sc=0;
    rand_t r0;
    rand_t r1;
    uint8_t *source;

    if (debug>0) {
        fprintf(stderr,"sender for %2d: start\n",p->destnode);
    }

    usleep(250000);
    rand_init(&r0,seed+mynode-1); // now nodes start fron ONE
    for (i=0;i<num;i++) {
        source=buffer;
        rand_init(&r1,rand_next_unsigned(&r0));
        *((unsigned int *)source)=r1.state;
        source+=sizeof(unsigned int);
        for (j=0;j<blocksize-sizeof(unsigned int);j++) {
            *source=rand_next(&r1);
            source++;
        }
        if (debug>1) {
            fprintf(stderr,"sending to %2d %5ld " MEMBLOCKFULLSTR "\n", p->destnode, sc++, MEMBLOCKFULLDATA(buffer,blocksize));
            if (debug>2) {
                dump(stderr, buffer,blocksize);
            }
        }
        ret=axiom_send_raw(dev, p->destnode, port, AXIOM_TYPE_RAW_DATA, blocksize, buffer);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_send_raw()");
        }
    }
    
    if (debug>0) {
        fprintf(stderr,"sender for %2d: end\n",p->destnode);
    }
    
    return NULL;    
}

static int errors=0;

void  *receiver(void *data) {
    axiom_msg_id_t id;
    axiom_type_t type;
    axiom_port_t myport;
    axiom_raw_payload_size_t size;
    axiom_node_id_t src_id;
    uint8_t *addr;
    uint8_t c;
    int i,j;
    int emit=0;
    uint8_t buffer[AXIOM_RAW_PAYLOAD_MAX_SIZE];
    long sc=0;
    rand_t r;

    const int endnum=(mode==MODE_MULTI?num*numnodes:num);
    if (debug>0) {
        fprintf(stderr,"receiver: start\n");
    }

    for (i=0;i<endnum;i++) {
        size=(axiom_raw_payload_size_t)sizeof(buffer);
        myport=port;
        id= axiom_recv_raw(dev, &src_id, &myport, &type, &size, buffer);
        if (!AXIOM_RET_IS_OK(id)) {
            perror("axiom_recv_raw()");
            continue;
        }
        if (size!=blocksize) {
            fprintf(stderr,"ERROR received from %2u %u bytes but expected %d bytes\n", (unsigned)src_id, (unsigned)size, blocksize);
            errors++;
        }
        addr=buffer;
        if (debug>1) {
            fprintf(stderr,"received from %2d %5ld " MEMBLOCKFULLSTR "\n", src_id, sc++, MEMBLOCKFULLDATA(addr,size));
            if (debug>2) {
                dump(stderr,addr,size);
            }
        }

        rand_init(&r,*((unsigned int *)addr));
        addr+=sizeof(unsigned int);
        emit=1;
        for (j=0;j<blocksize-sizeof(unsigned int);j++) {
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
    fprintf(stderr,"errors=%d\n",errors);

    if (debug>0) {
        fprintf(stderr,"receiver: end\n");
    }

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
    fprintf(stderr, "usage: testraw [options]\n");
    fprintf(stderr, "  -d|--debug     debug (every -d increase verbosiness, max 3)\n");
    fprintf(stderr, "  -s|--seed SEED seed for srand [default: %d]\n",SEED);
    fprintf(stderr, "  -p|--port PORT axiom port [default: %d]\n",PORT);
    fprintf(stderr, "  -n|--num NUM   numbers of block transmitted/received [default: %d]\n",NUM);
    fprintf(stderr, "  -b|--block SZ  block size in bytes [min:%ld default:%d max:%d]\n",sizeof(unsigned int)+1,BUFSIZE,AXIOM_RAW_PAYLOAD_MAX_SIZE);
    fprintf(stderr, "  -F|--full      full mode (i.e. every node send and receive fron only another node) [DEFAULT]\n");
    fprintf(stderr, "  -H|--half      half mode (i.e. every node send or receive but not both)\n");
    fprintf(stderr, "  -M|--multi     multi mode (i.e. every send to all other nodes)\n");
}

int main(int argc, char**argv) {
    int opt,long_index;
    axiom_err_t err;
    pthread_t threcv, thsend;
    pthread_t *thsenders=NULL;
    sender_data_t *pdata=NULL;
    int res,ret;

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hs:p:n:b:g:HMFd", long_options, &long_index)) != -1) {
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
                if (blocksize<sizeof(unsigned int)+1||blocksize>AXIOM_RAW_PAYLOAD_MAX_SIZE) {
                    fprintf(stderr, "ERROR: bad block size (-b option)\n");
                    help();
                    exit(EXIT_FAILURE);
                }
                break;
            case 'F':
                mode=MODE_FULL;
                break;
            case 'H':
                mode=MODE_HALF;
                break;
            case 'M':
                mode=MODE_MULTI;
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

    //
    // open & bind axiom device
    //

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
    numnodes=axiom_get_num_nodes(dev);
    
    //
    // nodes synchronization
    //

    ret=axrun_sync(7, 0);
    if (ret) {
        perror("axrun_sync()");
        exit(EXIT_FAILURE);
    }

    //
    // run working threads
    //

    if (mode==MODE_FULL||mode==MODE_MULTI||(mode==MODE_HALF&&(mynode&0x1)==0x0)) {
        res = pthread_create(&threcv, NULL, receiver, NULL);
        if (res != 0) {
            perror("pthread_create()");
            exit(EXIT_FAILURE);
        }
    }

    if (mode==MODE_FULL||(mode==MODE_HALF&&(mynode&0x1)==0x1)) {
        static sender_data_t data;
        data.destnode=yournode;
        res = pthread_create(&thsend, NULL, sender, &data);
        if (res != 0) {
            perror("pthread_create()");
            exit(EXIT_FAILURE);
        }
    } else if (mode==MODE_MULTI) {
        thsenders=malloc(sizeof(pthread_t)*numnodes);
        pdata=malloc(sizeof(sender_data_t)*numnodes);
        for (int k=0;k<numnodes;k++) {
            (pdata+k)->destnode=k+1; // now node are 1 based!
            res = pthread_create(thsenders+k, NULL, sender, pdata+k);
            if (res != 0) {
                perror("pthread_create()");
                exit(EXIT_FAILURE);
            }
        }
    }

    //
    // waiting threads
    //

    if (debug>0) {
        fprintf(stderr, "waiting...\n");
    }

    if (mode==MODE_FULL||(mode==MODE_HALF&&(mynode&0x1)==0x1)) {
        res = pthread_join(thsend, NULL);
        if (res != 0) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }
    } else if (mode==MODE_MULTI) {
        for (int k=0;k<numnodes;k++) {
            res = pthread_join(*(thsenders+k), NULL);
            if (res != 0) {
                perror("pthread_join()");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    if (mode==MODE_FULL||mode==MODE_MULTI||(mode==MODE_HALF&&(mynode&0x1)==0x0)) {
        res = pthread_join(threcv, NULL);
        if (res != 0) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }
    }

    //
    // done
    //
    
    axiom_close(dev);    
    return errors==0?EXIT_SUCCESS:EXIT_FAILURE;
}
