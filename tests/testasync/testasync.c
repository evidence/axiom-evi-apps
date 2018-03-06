/*!
 * \file testasync.c
 *
 * \version     v1.1
 *
 * A simple program to test axiom async rdma message exchange
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
    {"full", no_argument, 0, 'F'},
    {"half", no_argument, 0, 'H'},
    {"check", no_argument, 0, 'c'},
    {0, 0, 0, 0}
};

static axiom_dev_t *dev;
static int mynode,yournode;

#define MODE_FULL 0
#define MODE_HALF 1

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

typedef struct {
    axiom_token_t token;
    uint8_t *addr;
} checker_data_t ;

static volatile int done_counter=0;
static int check=0;
static int port=PORT;

void *checker(void *_data) {
    int ret;
    checker_data_t *data=(checker_data_t*)_data;

    if (debug>0)
        fprintf(stderr,"checker: start\n");
    if (debug>1)
        fprintf(stderr,"waiting token 0x%016lx...\n",data->token.raw);

    if (check) {
        for (;;) {
            ret=axiom_rdma_check(dev, &data->token, 1);
            if (ret == 1) break;
            if (ret == 0) continue;
            if (ret!=AXIOM_RET_NOTAVAIL) {
                perror("axiom_rdma_check()");
                usleep(1000000);
                continue;
            }
            usleep(125000);
        }
    } else {
        ret=axiom_rdma_wait(dev,&data->token,1);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_rdma_wait()");
        }
    }

    if (debug>1)
        fprintf(stderr,"token 0x%016lx arrived... sending remote notification...\n",data->token.raw);
    
    ret=axiom_send_raw(dev,yournode, port, AXIOM_TYPE_RAW_DATA, sizeof(data->addr), &data->addr);
    if (!AXIOM_RET_IS_OK(ret)) {
        perror("axiom_send_raw()");
    }

    // atomic increment of done conuter!
    __sync_fetch_and_add(&done_counter,1);

    if (debug>0)
        fprintf(stderr,"checker: end\n");

    free(data);
    return NULL;
}

static uint8_t *source,*source_other;
static int num=NUM;
static int blocksize=BUFSIZE;
static int guard=GUARD;
static unsigned seed=SEED;
static int mode=MODE_FULL;

void  *sender(void *data) {
    int i,j,ret;
    uint8_t *source_addr;
    axiom_token_t token;
    rand_t r;
    pthread_attr_t attr;
    pthread_t th;
    checker_data_t *mydata;

    if (debug>0)
        fprintf(stderr,"sender: start\n");

    ret=pthread_attr_init(&attr);
    if (ret!=0) {
        perror("pthread_attr_init()");
    }
    ret=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if (ret!=0) {
        perror("pthread_attr_setdetachstate()");
    }

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

        ret=axiom_rdma_write(dev, yournode, blocksize, source_addr, source_addr, &token);
        if (!AXIOM_RET_IS_OK(ret)) {
            perror("axiom_rdma_write()");
        }

        if (debug>1)
            fprintf(stderr,"starting waiting thread for 0x%016lx\n",token.raw);

        mydata=malloc(sizeof(checker_data_t));
        if (mydata==NULL) {
            perror("malloc()");
            continue;
        }
        mydata->token=token;
        mydata->addr=source_addr;
        ret=pthread_create(&th, &attr, checker, mydata);
        if (ret!=0) {
            perror("pthread_create()");
        }
    }

    if (debug>0)
        fprintf(stderr,"sender: end\n");
    
    return NULL;    
}

static int errors=0;

void  *receiver(void *data) {
    axiom_err_t err;
    axiom_port_t myport;
    axiom_raw_payload_size_t size;
    axiom_node_id_t src_id;
    axiom_type_t type;
    uint8_t *addr;
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
        for (;;) {
            err=axiom_recv_raw(dev, &src_id, &myport, &type, &size, (void*)&addr);
            if (err==AXIOM_RET_NOTAVAIL) {
                usleep(125000);
                continue;
            }
            break;
        }
        if (!AXIOM_RET_IS_OK(err)) {
            perror("axiom_recv_raw()");
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
        /* the check are at the end because the sender can send packets out of order */
/*
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
*/
    }

    // FINAL CHECK all memory and block guard
    rand_init(&r,seed+yournode);
    addr=source_other;
    for (i=0;i<num;i++) {
        emit=1;
        for (j=0;j<guard;j++) {
            if (*addr!=0) {
                if (emit) {
                    fprintf(stderr,"ERROR on guard block %d pos %4d: dirty, found 0x%02x\n",num,j,(unsigned)*addr);
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
                    fprintf(stderr,"ERROR on block %d pos %4d: expected 0x%02x found 0x%02x\n",num,j,(unsigned)c,(unsigned)*addr);
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
                fprintf(stderr,"ERROR on guard block %d pos %4d: dirty, found 0x%02x\n",num,j,(unsigned)*addr);
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
    fprintf(stderr, "usage: testasync [options]\n");
    fprintf(stderr, "  -d|--debug     debug (every -d increase verbosiness, max 3)\n");
    fprintf(stderr, "  -s|--seed SEED seed for srand [default: %d]\n",SEED);
    fprintf(stderr, "  -p|--port PORT axiom port [default: %d]\n",PORT);
    fprintf(stderr, "  -n|--num NUM   numbers of block transmitted/received [default: %d]\n",NUM);
    fprintf(stderr, "  -b|--block SZ  block size in bytes [min:%ld default:%d max:%d]\n",sizeof(unsigned int)+1,BUFSIZE,AXIOM_RDMA_PAYLOAD_MAX_SIZE);
    fprintf(stderr, "  -g|--guard SZ  guard block size [default: %d]\n",GUARD);
    fprintf(stderr, "  -F|--full      full mode (i.e. every node send and receive fron only another node) [DEFAULT]\n");
    fprintf(stderr, "  -H|--half      half mode (i.e. every node send or receive but not both)\n");
    fprintf(stderr, "  -c|--check     use axiom_rdma_check() not axiom_rdma_wait()\n");
}

int main(int argc, char**argv) {
    size_t privatesize;

    int opt,long_index;
    axiom_err_t err;
    pthread_t threcv, thsend;
    int res,ret;
    size_t sharedsize=0;
    uint8_t*base;
    
    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hs:p:n:b:g:dFHc", long_options, &long_index)) != -1) {
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
                    fprintf(stderr, "ERROR: bad block size (-b %s)\n",optarg);
                    help();
                    exit(EXIT_FAILURE);
                }

                break;
            case 'g':
                guard=asc2int(optarg);
                break;
            case 'F':
                mode=MODE_FULL;
                break;
            case 'H':
                mode=MODE_HALF;
                break;
            case 'c':
                check=1;
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

/*
    struct axiom_args openargs;
    openargs.flags = AXIOM_FLAG_NOBLOCK;
*/
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

    mymemset(base,0,SIZENEED);
    ret=axrun_sync(7, 0);
    if (ret) {
        perror("axrun_sync()");
        exit(EXIT_FAILURE);
    }

    if (mynode&0x1) {
        source_other=base;
        source=source_other+SIZENEED/2;
    } else {
        source=base;
        source_other=source+SIZENEED/2;
    }

    if (mode==MODE_FULL||(mode==MODE_HALF&&(mynode&0x1)==0x0)) {
        res = pthread_create(&threcv, NULL, receiver, NULL);
        if (res != 0) {
            perror("pthread_create()");
            exit(EXIT_FAILURE);
        }
    }

    if (mode==MODE_FULL||(mode==MODE_HALF&&(mynode&0x1)==0x1)) {
        res = pthread_create(&thsend, NULL, sender, NULL);
        if (res != 0) {
            perror("pthread_create()");
            exit(EXIT_FAILURE);
        }
    }
    
    if (debug>0)
        fprintf(stderr, "waiting...\n");

    if (mode==MODE_FULL||(mode==MODE_HALF&&(mynode&0x1)==0x1)) {
        res = pthread_join(thsend, NULL);
        if (res != 0) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }

        /* wait for all checker thread (we does not want to join every thread!) */
        while (__sync_fetch_and_add(&done_counter,0)!=num) {
          usleep(500000);
        };
    }
    
    if (mode==MODE_FULL||(mode==MODE_HALF&&(mynode&0x1)==0x0)) {
        res = pthread_join(threcv, NULL);
        if (res != 0) {
            perror("pthread_join()");
            exit(EXIT_FAILURE);
        }
    }
    
    axiom_close(dev);    
    return errors==0?EXIT_SUCCESS:EXIT_FAILURE;
}
