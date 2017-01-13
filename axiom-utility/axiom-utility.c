/*!
 * \file axiom-utility.c
 *
 * \version     v0.10
 * \date        2016-09-29
 *
 * This file contains the implementation of axiom-utility application.
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_init.h"

int verbose = 0;

static void usage(void) {
    printf("usage: axiom-utility [arguments]\n");
    printf("Various utility\n\n");
    printf("Arguments:\n");
    printf("-f, --flush       flush all messages from every not already binded port\n");
    printf("-v, --verbose     verbose\n");
    printf("-h, --help        print this help\n\n");
}

static inline void exec_flush(void) {
    axiom_args_t axiom_args;
    axiom_port_t port;
    axiom_err_t err;
    axiom_node_id_t node;
    axiom_type_t type;
    size_t size;
    uint8_t buffer[AXIOM_LONG_PAYLOAD_MAX_SIZE];
    axiom_dev_t *dev=NULL;
    int counter,oldcounter;

    axiom_args.flags |= AXIOM_FLAG_NOFLUSH;
    for (port=0;port<AXIOM_PORT_MAX;port++) {
        if (dev!=NULL) axiom_close(dev);
        dev=axiom_open(&axiom_args);
        if (dev==NULL) {
            perror("axiom_open() return NULL");
            exit(-1);
        }
        err=axiom_bind(dev,port);
        if (!AXIOM_RET_IS_OK(err)) {
            if (verbose) printf("skipped port %i\n",port);
            continue;
        }
        if (verbose) {
            printf("flushing port %i... ",port);
            fflush(stdout);
        }
        oldcounter=-1;
        counter=0;
        for (;;) {
            while (axiom_recv_raw_avail(dev)||axiom_recv_long_avail(dev)) {
                size=AXIOM_LONG_PAYLOAD_MAX_SIZE;
                axiom_recv(dev,&node,&port,&type,&size,buffer);
                counter++;
            }
            if (counter==oldcounter) break;
            oldcounter=counter;
            usleep(250000);
        }
        if (verbose) printf("flushed (found %d lost packets)!\n",counter);
    }
    if (dev!=NULL) axiom_close(dev);
}

int main(int argc, char **argv) {
    int long_index =0;
    int opt = 0;
    int flush=0;

    static struct option long_options[] = {
            {"flush", no_argument,       0, 'f'},
            {"verbose", no_argument,     0, 'v'},
            {"help", no_argument,        0, 'h'},
            {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv,"hop:s:nft:m:i:v", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'f':
                flush = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                usage();
                exit(-1);
            default:
                printf("ERROR: invalid option -%c\n",opt);
                usage();
                exit(-1);
        }
    }

    if (flush) {
        exec_flush();
    } else {
        printf("ERROR: no utility function requested\n");
        usage();
        exit(-1);
    }

    return 0;
}
