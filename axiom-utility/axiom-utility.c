/*!
 * \file axiom-utility.c
 *
 * \version     v1.1
 * \date        2016-09-29
 *
 * This file contains the implementation of axiom-utility application.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <sys/socket.h>
#include <sys/un.h>
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
#include "axiom_nic_raw_commands.h"

int verbose = 0;

static void usage(void) {
    printf("usage: axiom-utility [arguments]\n");
    printf("Various utility\n");
    printf("Version: %s\n", AXIOM_API_VERSION_STR);
    printf("\n\n");
    printf("Arguments:\n");
    printf("-f, --flush       flush all messages from every not already binded port\n");
    printf("-m, --master      tell local axiom-init to start a network discovery as master\n");
    printf("-v, --verbose     verbose\n");
    printf("-V, --version     print version\n");
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

    axiom_args.flags = AXIOM_FLAG_NOFLUSH;
    for (port=0;port<=AXIOM_PORT_MAX;port++) {
        if (dev!=NULL) axiom_close(dev);
        dev=axiom_open(&axiom_args);
        if (dev==NULL) {
            perror("axiom_open() return NULL");
            exit(-1);
        }
        err=axiom_bind(dev,port);
        if (!AXIOM_RET_IS_OK(err)) {
            if (verbose) printf("skipped port %d\n",(int)port);
            continue;
        }
        if (verbose) {
            printf("flushing port %d... ",(int)port);
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

static inline void exec_start_netwok_discovery(void) {
    struct sockaddr_un itsaddr;
    struct msghdr msg;
    struct iovec iov;
    int sock;
    axiom_init_payload_t payload;
    int res;

    if (verbose) printf("opening unix domanin socket to comunicat with axiom-init...\n");
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("creating socket");
        return;
    }
    itsaddr.sun_family = AF_UNIX;
    snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), AXIOM_INIT_SOCKET_PATHNAME);

    payload.command=AXIOM_CMD_START_DISCOVERY;
    iov.iov_base=&payload;
    iov.iov_len=sizeof(payload.command);
    memset(&msg,0,sizeof(msg));
    msg.msg_name=(void*)&itsaddr;
    msg.msg_namelen=sizeof(itsaddr);
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
    if (verbose) printf("sending AXIOM_CMD_START_DISCOVERY to axiom-init...\n");
    res=sendmsg(sock,&msg,0);
    if (res != sizeof(payload.command)) {
        perror("sending message");
        close(sock);
        return;
    }
    
    if (verbose) printf("closing socket...\n");
    close(sock);
    return;
}

#define CMD_NO_COMMAND 0
#define CMD_FLUSH      1
#define CMD_MASTER     2

int main(int argc, char **argv) {
    int long_index =0;
    int opt = 0;
    int cmd=CMD_NO_COMMAND;

    static struct option long_options[] = {
            {"flush", no_argument,       0, 'f'},
            {"master", no_argument,       0, 'm'},
            {"verbose", no_argument,     0, 'v'},
            {"version", no_argument,     0, 'V'},
            {"help", no_argument,        0, 'h'},
            {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv,"fmvVh", long_options,
                    &long_index )) != -1) {
        switch (opt) {
            case 'f':
                cmd = CMD_FLUSH;
                break;
            case 'm':
                cmd = CMD_MASTER;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                usage();
                exit(-1);
            case 'V':
                printf("Version: %s\n", AXIOM_API_VERSION_STR);
                exit(0);
            default:
                printf("ERROR: invalid option -%c\n",opt);
                usage();
                exit(-1);
        }
    }

    switch (cmd) {
        case CMD_FLUSH:
            exec_flush();
            break;
        case CMD_MASTER:
            exec_start_netwok_discovery();
            break;
        default:
            printf("ERROR: no utility function requested\n");
            usage();
            exit(-1);
    }

    return 0;
}
