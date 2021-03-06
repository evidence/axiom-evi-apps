/*!
 * \file testlong.c
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

#include "../common/test.h"

int debug=0;

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"seed", required_argument, 0, 's'},
    {"num", required_argument, 0, 'n'},
    {"block", required_argument, 0, 'b'},
    {"debug", required_argument, 0, 'd'},
    {"receivers", required_argument, 0, 'r'},
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
#define BUFSIZE 2442
// receiver delay (usec)
#define RECEIVER_DELAY 0

typedef struct {
    int destnode;
} sender_data_t ;

static int num=NUM;
static int blocksize=BUFSIZE;
static int mode=MODE_FULL;
static int port=PORT;
static int seed=SEED;
static int long rdelay=RECEIVER_DELAY;

void  *sender(void *data) {
    sender_data_t *p=(sender_data_t *)data;
    uint8_t buffer[AXIOM_LONG_PAYLOAD_MAX_SIZE];
    int i,j,k,ret;
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
        k=0;
        while ((ret=axiom_send_long(dev, p->destnode, port, blocksize, buffer))==AXIOM_RET_NOTAVAIL) {
            if (++k>16) break;
            usleep(k>8?250000:125000);
        }
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_send_long()");
        }
    }
    
    if (debug>0) {
        fprintf(stderr,"sender for %2d: end\n",p->destnode);
    }
    
    return NULL;    
}

static int errors=0;
static volatile int received=0;

void  *receiver(void *data) {
    axiom_err_t err;
    axiom_port_t myport;
    axiom_long_payload_size_t size;
    axiom_node_id_t src_id;
    uint8_t *addr;
    uint8_t c;
    int j;
    int emit=0;
    uint8_t buffer[AXIOM_LONG_PAYLOAD_MAX_SIZE];
    long sc=0;
    rand_t r;
    long mydelay=0;

    const int endnum=(mode==MODE_MULTI?num*numnodes:num);
    if (debug>0) {
        fprintf(stderr,"receiver: start\n");
    }

    while (endnum>__sync_fetch_and_or(&received,0)) {
        size=sizeof(buffer);
        myport=port;
        if (mydelay>0) {
            usleep(mydelay);
            mydelay=0;
        }
        err= axiom_recv_long(dev, &src_id, &myport, &size, buffer);
        if (err==AXIOM_RET_NOTAVAIL) continue;
        if (!AXIOM_RET_IS_OK(err)) {
            perror("axiom_recv_long()");
            continue;
        }
        mydelay=rdelay;
        __sync_fetch_and_add(&received,1);
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
    fprintf(stderr, "usage: testlong [options]\n");
    fprintf(stderr, "  -d|--debug          debug (every -d increase verbosiness, max 3)\n");
    fprintf(stderr, "  -s|--seed SEED      seed for srand [default: %d]\n",SEED);
    fprintf(stderr, "  -p|--port PORT      axiom port [default: %d]\n",PORT);
    fprintf(stderr, "  -n|--num NUM        numbers of block transmitted/received [default: %d]\n",NUM);
    fprintf(stderr, "  -b|--block SZ       block size in bytes [min:%ld default:%d max:%d]\n",sizeof(unsigned int)+1,BUFSIZE,AXIOM_LONG_PAYLOAD_MAX_SIZE);
    fprintf(stderr, "  -F|--full           full mode (i.e. every node send and receive fron only another node) [DEFAULT]\n");
    fprintf(stderr, "  -H|--half           half mode (i.e. every node send or receive but not both)\n");
    fprintf(stderr, "  -M|--multi          multi mode (i.e. every send to all other nodes)\n");
    fprintf(stderr, "  -k|--noblock        open device in NOT BLOCKING operation mode\n");
    fprintf(stderr, "  -r|--receivers NUM  number of receivers thread [default: 1]\n");
    fprintf(stderr, "  -D|--delay NUM      receiver delay after each successfull message (in msec) [default: 0]\n");
    fprintf(stderr, "note:\n");
    fprintf(stderr, "  if you use more than one receivers (i.e. -r NUM with NUM>1) and a blocking operation (i.e. no -k specified) than the test should not terminate!");
}

int main(int argc, char**argv) {
    struct axiom_args openargs;
    int opt,long_index;
    axiom_err_t err;
    pthread_t thsend;
    pthread_t *threcv=NULL;
    pthread_t *thsenders=NULL;
    sender_data_t *pdata=NULL;
    int res,ret;
    int receivers=1;
    int c;

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hs:p:n:b:g:HMFdkr:D:", long_options, &long_index)) != -1) {
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
            case 'r':
                receivers=asc2int(optarg);
                break;
            case 'D':
                rdelay=asc2long(optarg)*1000;
                break;
            case 'b':
                blocksize=asc2int(optarg);
                if (blocksize<sizeof(unsigned int)+1||blocksize>AXIOM_LONG_PAYLOAD_MAX_SIZE) {
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
            case 'k':
                openargs.flags = AXIOM_FLAG_NOBLOCK;
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
    dev = axiom_open(&openargs);
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
        threcv=malloc(receivers*sizeof(pthread_t));
        if (threcv==NULL) {
            perror("malloc()");
            exit(EXIT_FAILURE);
        }
        for (c=0;c<receivers;c++) {
            res = pthread_create(threcv+c, NULL, receiver, NULL);
            if (res != 0) {
                perror("pthread_create()");
                exit(EXIT_FAILURE);
            }
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
        for (c=0;c<receivers;c++) {
            res = pthread_join(threcv[c], NULL);
            if (res != 0) {
                perror("pthread_join()");
                exit(EXIT_FAILURE);
            }
        }
    }

    //
    // done
    //
    
    axiom_close(dev);    
    return errors==0?EXIT_SUCCESS:EXIT_FAILURE;
}
