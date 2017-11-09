/*!
 * \file myrpc.c
 *
 * \version     v0.15
 *
 * A simple program to test axiom-run RPC
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "axiom_run_api.h"

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"sleep", required_argument, 0, 's'},
    {0, 0, 0, 0}
};

void mysigquit(int dummy) {
    fprintf(stderr, "SIGQUIT called\n");
    exit(EXIT_FAILURE);
}

#define MAXBUFSIZE 6

int bufout[MAXBUFSIZE];
int bufin[MAXBUFSIZE];

int main(int argc, char**argv) {
    useconds_t delay = 0;
    int opt, long_index, res;
    double n;
    size_t size;
    int i;

    signal(SIGQUIT, mysigquit);

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hs:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 's':
                n = strtod(optarg, NULL);
                delay = (useconds_t) (n * 1e6);
                break;
            case 'h':
            case '?':
                fprintf(stderr, "usage: myrpc [ARGS]*\n");
                fprintf(stderr, "where ARGS\n");
                fprintf(stderr, "  -h                is this help screen\n");
                fprintf(stderr, "  -s|--sleep SEC    initial delay in seconds\n");
                exit(0);
        }
    }

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    if (delay > 0) {
        fprintf(stdout, "initial delay...\n");
        usleep(delay);
    }
    fprintf(stdout, "fillin buffer ...\n");
    size=MAXBUFSIZE*sizeof(int);
    for (i=0;i<MAXBUFSIZE;i++) {
        bufout[i]=getpid()+i;
        bufin[i]=0;
    }
    fprintf(stdout, "axrun_rpc() called for AXRUN_RPC_PING...\n");
    res = axrun_rpc(AXRUN_RPC_PING,MAXBUFSIZE*sizeof(int),bufout,&size,bufin,1);
    if (res!=0) {
        fprintf(stdout,"ERROR: axrun_rpc() return %d!\n",res);
    } else {
        if (size!=MAXBUFSIZE*sizeof(int)) {
            fprintf(stdout,"ERROR: message size mismatch (expected %d received %d)!\n",(int)(MAXBUFSIZE*sizeof(int)),(int)size);
        }  else {
            for (i=0;i<MAXBUFSIZE;i++) {
                if (bufin[i]!=bufout[i]) {
                    fprintf(stdout,"ERROR: message data (index=%d expecetd=%d received=%d)!\n",i,bufout[i],bufin[i]);
                    break;
                }
            }
            fprintf(stderr,"SUCCESFULL: replay received\n");
        }
    }
    return EXIT_SUCCESS;
}
