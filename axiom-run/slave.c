/**
 * Manage services for axion-run slave process.
 *
 * @file slave.c
 * @version v0.7
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "axiom-run.h"

#ifndef UNIX_PATH_MAX
// safe default
#define UNIX_PATH_MAX 108
#endif

typedef struct {
    axiom_dev_t *dev;
    uint64_t services;
    int fd;
    uint8_t cmd;
    pid_t pid;

    int sock;
    struct sockaddr_un youraddr;
} thread_info_t;

/**
 * Thread to manage output redirect service.
 *
 * @param data needed information
 * @return don't care
 */
static void *send_thread(void *data) {
    thread_info_t *info = (thread_info_t*) data;
    axiom_msg_id_t msg;
    buffer_t buffer;
    size_t sz;
    char *id = (info->cmd == CMD_SEND_TO_STDOUT) ? "STDOUT" : "STDERR";

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: redirect loop for %s started", id);
    buffer.header.command = ((thread_info_t*) data)->cmd;
    //
    // main loop
    // (exit in case of read failure)
    for (;;) {
        // read slave stdout/stderr...
        sz = read(info->fd, buffer.raw, sizeof (buffer.raw));
        zlogmsg(LOG_TRACE, LOGZ_SLAVE, "SLAVE: %s read %d bytes from fd", id, (int) sz);
        if (sz == -1) {
            if (errno == EINTR) continue; // paranoia
            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: %s thread read error (errno=%d '%s')", id, errno, strerror(errno));
            break;
        }
        // send data read to axiom-run master...
        if (sz > 0) {
            msg = axiom_send_raw(info->dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sz + sizeof (header_t), &buffer);
            if (!AXIOM_RET_IS_OK(msg))
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: %s thread axiom_send_raw() write error (err=%d)", id, msg);
        }
    }
    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: redirect loop for %s end", id);
    return NULL;
}

/**
 * Write buffer to file.
 * Used by the redirect stdin service.
 * 
 * @param fd file descriptor
 * @param buf the buffer
 * @param size the buffer size
 */
static void output(int fd, uint8_t *buf, int size) {
    int sz;
    while (size > 0) {
        sz = write(fd, buf, size);
        if (sz < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (sz == 0) continue;
        size -= sz;
        buf += sz;
    }
    if (size != 0) {
        zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: write to pipe error (errno=%d)", errno);
    }
}

/**
 * Thread to manage message form axiom-run master.
 *
 * @param data data needed
 * @return don't care
 */
static void *recv_thread(void *data) {
    thread_info_t *info = (thread_info_t*) data;
    axiom_node_id_t node;
    axiom_port_t port;
    axiom_type_t type;
    buffer_t buffer;
    axiom_msg_id_t msg;
    axiom_raw_payload_size_t size;
    struct sockaddr_un itsaddr;
    int sock, res;

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: receiver thread started");

    if (info->services & BARRIER_SERVICE) {
        // socket used to inform the child process of barrier synchronization...
        sock = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sock == -1) {
            elogmsg("socket()");
            exit(EXIT_FAILURE);
        }
        itsaddr.sun_family = AF_UNIX;
    }

    // this is needed because axiom_recv_raw is not a cancellation point and is blocking...
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //
    // MAIN LOOP
    // (forever)
    //
    for (;;) {
        // wait master messages...
        size = sizeof (buffer);
        msg = axiom_recv_raw(info->dev, &node, &port, &type, &size, &buffer);
        if (!AXIOM_RET_IS_OK(msg)) {
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: receiver thread error into axiom_recv_raw() %d", msg);
            continue;
        }
        zlogmsg(LOG_TRACE, LOGZ_SLAVE, "SLAVE: received %d bytes command=%d", size, buffer.header.command);
        if (buffer.header.command == CMD_RECV_FROM_STDIN) {
            //
            // manage stdin redirect service
            //
            if (info->services & REDIRECT_SERVICE) {
                size -= sizeof (header_t);
                output(info->fd, buffer.raw, size);
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_RECV_FROM_STDIN");
            }
        } else if (buffer.header.command == CMD_EXIT) {
            //
            // manage exit service
            //
            if (info->services & EXIT_SERVICE) {
                zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: received CMD_EXIT");
                //received_exit_command = 1;
                res = kill(info->pid, SIGQUIT);
                if (res != 0) {
                    zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: error sending SIGQUIT signal to controlled application errno=%d '%s'!", errno, strerror(errno));
                }
                //exit(EXIT_SUCCESS);
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_EXIT");
            }
        } else if (buffer.header.command == CMD_BARRIER) {
            //
            // manage barrier service
            //
            if (info->services & BARRIER_SERVICE) {
                snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), "/tmp/axbar%d.%d", (int) getpid(), buffer.header.barrier_id);
                res = sendto(sock, &buffer.header.barrier_id, sizeof (unsigned), 0, (struct sockaddr*) &itsaddr, sizeof (itsaddr));
                if (res != sizeof (unsigned)) {
                    zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: receiver thread sendto() error (errno=%d '%s')!", errno, strerror(errno));
                }
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_BARRIER");
            }
        } else {
            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: unknown message command 0x%02x", buffer.header.command);
        }
    }
    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: receiver thread end");
    return NULL;
}

/**
 * Thread to manage barrier service.
 * @param data data needed
 * @return don't care
 */
static void *sock_thread(void *data) {
    thread_info_t *info = (thread_info_t*) data;
    axiom_msg_id_t msg;
    buffer_t buffer;
    struct sockaddr_un myaddr;
    int sock, res;
    unsigned id;

    // read synchornization request from a socket and send the request to the master...

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: socket thread started");

    // socket used for slave<->child comunnication
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) {
        elogmsg("socket()");
        exit(EXIT_FAILURE);
    }
    myaddr.sun_family = AF_UNIX;
    snprintf(myaddr.sun_path, sizeof (myaddr.sun_path), "/tmp/axbar%d", (int) getpid());
    res = bind(sock, &myaddr, sizeof (myaddr));
    if (res == -1) {
        elogmsg("bind()");
        exit(EXIT_FAILURE);
    }

    buffer.header.command = CMD_BARRIER;
    //
    // MAIN LOOP
    // (forever)
    //
    for (;;) {
        // receive data from child...
        res = recv(sock, &id, sizeof (unsigned), MSG_WAITALL);
        if (res == -1 && errno == EAGAIN) continue;
        if (res == -1) {
            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: socket thread recv error (errno=%d '%s')", errno, strerror(errno));
            continue;
        }
        if (res != sizeof (unsigned)) {
            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: socket thread recv bad message");
            continue;
        }
        // send request to master...
        buffer.header.barrier_id = id;
        msg = axiom_send_raw(info->dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sizeof (header_t), &buffer);
        if (!AXIOM_RET_IS_OK(msg))
            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: socket thread axiom_send_raw() write error (err=%d)", msg);
    }
    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: socket thread end");
    return NULL;
}

/* these are necessary for the signal handler :-( */
static int mypid;
static axiom_dev_t *mydev;

/**
 * Exit signal handler.
 *
 * - kills the child
 * - send EXIT message to the master
 * - unlinks the named unix domani socket
 * - exit
 *
 * @param sig signal number
 */
static void myexit(int sig) {
    axiom_msg_id_t msgid;
    buffer_t buffer;
    int res;
    szlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: caught exit signal... exiting...");
    //
    // kill child
    //
    if (mypid > 0) {
        res = kill(mypid, SIGQUIT);
        if (res != 0) {
            szlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: error sending SIGQUIT signal from signal handler to controlled application errno=%d '%s'!", errno, strerror(errno));
        }
    } else {
        szlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: mypid is <0!!!!");
    }
    //
    // unlink socket (used by barrier service)
    //
    {
        // paranoia
        char sname[UNIX_PATH_MAX];
        snprintf(sname, sizeof (sname), "/tmp/axbar%d", (int) getpid());
        unlink(sname);
    }
    //
    // notify master the termination
    //
    if (mydev != NULL) {
        buffer.header.command = CMD_EXIT;
        buffer.header.status = sig & 0x7f; // HACK: works... for now.... see <bits/waitstatus.h>
        msgid = axiom_send_raw(mydev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sizeof (header_t), &buffer);
        if (!AXIOM_RET_IS_OK(msgid)) {
            szlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: sending CMD_EXIT to master from signal handler errror res=%d", msgid);
        }
    } else {
        szlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: mydev is NULL!!!!");
    }
    exit(EXIT_SUCCESS);
}

/* see axiom-run.h */
void manage_slave_services(axiom_dev_t *_dev, int _services, int *_fd, pid_t _pid) {
    thread_info_t forout, forerr, forin, forsock;
    pthread_t thout, therr, thin, thsock;
    //pthread_attr_t attr;
    sigset_t oldset;
    pid_t resp;
    int res;
    int status;
    buffer_t buffer;
    axiom_msg_id_t msgid;

    forsock.dev = forout.dev = forerr.dev = forin.dev = _dev;
    forsock.services = forout.services = forerr.services = forin.services = _services;
    forsock.pid = forout.pid = forerr.pid = forin.pid = _pid;

    if (_services && _pid > 0) {

        //
        // start service threads...
        //
        zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: starting service threads");

        // block all signal (so thread started have the mask set)
        block_all_signals(&oldset);

        if (_services & REDIRECT_SERVICE) {
            forout.cmd = CMD_SEND_TO_STDOUT;
            forout.fd = _fd[1];
            res = pthread_create(&thout, NULL, send_thread, &forout);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
            forerr.cmd = CMD_SEND_TO_STDERR;
            forerr.fd = _fd[2];
            res = pthread_create(&therr, NULL, send_thread, &forerr);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
        }
        if ((_services & REDIRECT_SERVICE) || (_services && EXIT_SERVICE)) {
            forin.cmd = 0;
            forin.fd = ((_services & REDIRECT_SERVICE) ? _fd[0] : -1);
            res = pthread_create(&thin, NULL, recv_thread, &forin);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
        }
        if (_services & BARRIER_SERVICE) {
            res = pthread_create(&thsock, NULL, sock_thread, &forsock);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
        }

        // restore main thread signal mask and set exit signal handler,,,
        mydev = _dev;
        mypid = _pid;
        restore_signals_and_set_quit_handler(&oldset, myexit);
    }

    if (_pid > 0) {
        //
        // wait termination of child process...
        //)
        zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: waiting child process...");
        for (;;) {
            resp = waitpid(_pid, &status, 0);
            if (resp == _pid) break;
            if (resp == -1 && errno == EINTR) continue;
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: waitpid error res=%d '%s' errno=%d", (int) resp, strerror(errno), errno);
            break;
        }
    }

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: child process %s...", _pid > 0 ? "died" : "never started");

    if (_services && _pid > 0) {
        //
        // end service threads...
        //
        zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: waiting working threads...");
        if (_services & REDIRECT_SERVICE) {
            terminate_thread_slave(thout);
            terminate_thread_slave(therr);
        }
        if ((_services & REDIRECT_SERVICE) || (_services && EXIT_SERVICE)) {
            terminate_thread_slave(thin);
        }
        if (_services & BARRIER_SERVICE) {
            char sname[UNIX_PATH_MAX];
            terminate_thread_slave(thsock);
            snprintf(sname, sizeof (sname), "/tmp/axbar%d", (int) getpid());
            unlink(sname);
        }
        zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: working threads died");
    }

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: send CMD_EXIT message to master");

    //
    // send master child termination notification...
    //
    buffer.header.command = CMD_EXIT;
    buffer.header.status = status;
    msgid = axiom_send_raw(_dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sizeof (header_t), &buffer);
    if (!AXIOM_RET_IS_OK(msgid)) {
        zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: sending CMD_EXIT to master errror res=%d", msgid);
    }
}
