/*!
 * \file rpc.c
 *
 * \version     v0.12
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../common.h"
#include "axiom_run_api.h"
#include "lib.h"

int axrun_rpc(int function, size_t send_size, void *send_payload, size_t *recv_size, void *recv_payload, int verbose) {
    struct sockaddr_un myaddr, itsaddr;
    int sock, res;
    int line;
    char *s;
    int ppid;
    struct msghdr msg;
    struct iovec iov[2];
    header_t header;
    uint64_t tid=gettid();

    sock = -1;
    myaddr.sun_path[0] = '\0';

    //
    // search pid of the barrier service
    //
    s = getenv("AXIOM_USOCK");
    if (s == NULL) {
        line = __LINE__;
        errno = EINVAL;
        goto error;
    }
    ppid = (pid_t) atoi(s);

    //
    // create a Unix domain socket
    // and bind it to axbar$PARENT_PID.$BARRIER_ID
    //
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) goto error;
    myaddr.sun_family = AF_UNIX;
    snprintf(myaddr.sun_path, sizeof (myaddr.sun_path), RPC_CHILD_TEMPLATE_NAME, ppid, tid);
    res = bind(sock, (struct sockaddr *) &myaddr, sizeof (myaddr));
    if (res == -1) {
        line = __LINE__;
        goto error;
    }

    //
    // send RPC to axbar$PARENT_PID
    //
    itsaddr.sun_family = AF_UNIX;
    snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), SLAVE_TEMPLATE_NAME, (int) ppid);

    header.command=CMD_RPC;
    header.rpc.id=tid;
    header.rpc.function=function;
    header.rpc.size=send_size;
    iov[0].iov_base=&header;
    iov[0].iov_len=sizeof(header);
    iov[1].iov_base=send_payload;
    iov[1].iov_len=send_size;
    msg.msg_name=(void*)&itsaddr;
    msg.msg_namelen=sizeof(itsaddr);
    msg.msg_iov=iov;
    msg.msg_iovlen=2;
    msg.msg_control=NULL;
    msg.msg_controllen=0;
    msg.msg_flags=0;
    res=sendmsg(sock,&msg,0);
    if (res != sizeof(header)+send_size) {
        line = __LINE__;
        goto error;
    }

    //
    // wait the replay from the parent (blocking!)
    //
    if (recv_payload!=NULL&&*recv_size>0) {
        msg.msg_name=NULL;
        msg.msg_namelen=0;
        iov[1].iov_base=recv_payload;
        iov[1].iov_len=*recv_size;
        res = recvmsg(sock, &msg, MSG_WAITALL);
        if (res == -1) goto error;
        *recv_size=res-sizeof(header_t);
    }

    //
    // release resource and return
    //
    close(sock);
    unlink(myaddr.sun_path);
    return 0;

    //
    // in case of error
    // release resources
error:
    if (verbose) {
        char msg[256];
        snprintf(msg, sizeof (msg), "axrun_rpc() errno %d at line %d", errno, line);
        perror(msg);
    }
    if (sock != -1) close(sock);
    if (*myaddr.sun_path != '\0') unlink(myaddr.sun_path);

    return -1;
}
