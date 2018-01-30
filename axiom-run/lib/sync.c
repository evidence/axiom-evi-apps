/*!
 * \file sync.c
 *
 * \version     v0.15
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
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

/* see axiom_run_api.h */
int axrun_sync(const unsigned barrier_id, int verbose) {
    struct sockaddr_un myaddr, itsaddr;
    int sock, res;
    int line=__LINE__;
    char *s;
    pid_t ppid;
    header_t header;

    sock = -1;
    myaddr.sun_path[0] = '\0';

    if (barrier_id > AXRUN_MAX_BARRIER_ID) {
        line = __LINE__;
        errno = EINVAL;
        goto error;
    }

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
    snprintf(myaddr.sun_path, sizeof (myaddr.sun_path), BARRIER_CHILD_TEMPLATE_NAME, (int) ppid, barrier_id);
    res = bind(sock, (struct sockaddr *) &myaddr, sizeof (myaddr));
    if (res == -1) {
        line = __LINE__;
        goto error;
    }
    //
    // send the barrier id to axbar$PARENT_PID
    //
    itsaddr.sun_family = AF_UNIX;
    snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), SLAVE_TEMPLATE_NAME, (int) ppid);
    header.command=CMD_BARRIER;
    header.barrier.barrier_id=barrier_id;
    res = sendto(sock, &header, sizeof (header), 0, (struct sockaddr*) &itsaddr, sizeof (itsaddr));
    if (res != sizeof (header)) {
        line = __LINE__;
        goto error;
    }
    //
    // wait the replay from the parent (blocking!)
    //
    res = recv(sock, &header, sizeof (header), MSG_WAITALL);
    if (res == -1) goto error;
    if (header.barrier.barrier_id != barrier_id || header.command!=CMD_BARRIER) {
        line = __LINE__;
        errno = ENOMSG;
        goto error;
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
        snprintf(msg, sizeof (msg), "axrun_sync() errno %d at line %d", errno, line);
        perror(msg);
    }
    if (sock != -1) close(sock);
    if (*myaddr.sun_path != '\0') unlink(myaddr.sun_path);

    return -1;
}
