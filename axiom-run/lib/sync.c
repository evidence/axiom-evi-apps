
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "../axiom-run.h"
#include "axiom_run_api.h"

/* see axiom_run_api.h */
int axrun_sync(const unsigned barrier_id, int verbose) {
    struct sockaddr_un myaddr, itsaddr;
    unsigned id;
    int sock, res;
    int line;

    sock = -1;
    myaddr.sun_path[0] = '\0';

    if (barrier_id > AXRUN_MAX_BARRIER_ID) {
        line = __LINE__;
        errno = EINVAL;
        goto error;
    }
    //
    // create a Unix domain socket
    // and bind it to axbar$PARENT_PID.$BARRIER_ID
    //
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) goto error;
    myaddr.sun_family = AF_UNIX;
    snprintf(myaddr.sun_path, sizeof (myaddr.sun_path), BARRIER_SLAVE_TEMPLATE_NAME, (int) getppid(), barrier_id);
    res = bind(sock, &myaddr, sizeof (myaddr));
    if (res == -1) {
        line = __LINE__;
        goto error;
    }
    //
    // send the barrier id to axbar$PARENT_PID
    //
    itsaddr.sun_family = AF_UNIX;
    snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), BARRIER_MASTER_TEMPLATE_NAME, (int) getppid());
    res = sendto(sock, &barrier_id, sizeof (unsigned), 0, (struct sockaddr*) &itsaddr, sizeof (itsaddr));
    if (res != sizeof (unsigned)) {
        line = __LINE__;
        goto error;
    }
    //
    // wait the replay from the parent (blocking!)
    //
    res = recv(sock, &id, sizeof (unsigned), MSG_WAITALL);
    if (res == -1) goto error;
    if (id != barrier_id) {
        line = __LINE__;
        errno = ENOMSG;
        goto error;
    }
    //
    // release resource and return
    //
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
