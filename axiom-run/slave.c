/*!
 * \file slave.c
 *
 * \version     v0.11
 *
 * Manage services for axion-run slave process.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <sys/select.h>
#include <sys/eventfd.h>
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

    int endfd;

    int termmode;
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
    fd_set set;
    int res, maxfd;
    char *id = (info->cmd == CMD_SEND_TO_STDOUT) ? "STDOUT" : "STDERR";

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: redirect loop for %s started (thread=%ld)", id, (long) pthread_self());
    buffer.header.command = ((thread_info_t*) data)->cmd;
    //
    // main loop
    // (exit in case of read failure)
    maxfd=info->endfd>info->fd?info->endfd+1:info->fd+1;
    for (;;) {
        // read slave stdout/stderr...
        zlogmsg(LOG_TRACE, LOGZ_SLAVE, "SLAVE: %s waiting data...", id);
        FD_ZERO(&set);
        FD_SET(info->fd,&set);
        FD_SET(info->endfd,&set);
        res=select(maxfd,&set,NULL,NULL,NULL);
        if (res==-1) {
            if (errno == EINTR) continue; // paranoia
            elogmsg("select() on send_thread thread");
            break;
        }
        if (FD_ISSET(info->endfd,&set)) {
            eventfd_t value;
            eventfd_read(info->endfd,&value); // not really needed
            zlogmsg(LOG_DEBUG, LOGZ_MASTER, "SLAVE: redirect loop for %s received termination request",id);
            break;
        }
        sz = read(info->fd, buffer.raw, sizeof (buffer.raw));
        zlogmsg(LOG_TRACE, LOGZ_MASTER, "SLAVE: SEND_THREAD: read() %d bytes for %s", (int)sz,id);
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
        } else {
            break;
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

/* if I kill my slave process does not use SIGTERM as exit value ! */
static int my_sigterm=0;

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
    axiom_err_t err;
    int maxfd,rawfd;
    fd_set set;
    
    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: receiver thread started (thread=%ld)", (long) pthread_self());

    if (info->services & (BARRIER_SERVICE|RPC_SERVICE)) {
        // socket used to inform the child process of barrier synchronization...
        sock = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sock == -1) {
            elogmsg("socket()");
            exit(EXIT_FAILURE);
        }
        itsaddr.sun_family = AF_UNIX;
    }

    // this is needed because axiom_recv_raw is not a cancellation point and is blocking...
    // PS: not used anymore but keepd for failsafe
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    //
    // MAIN LOOP
    // (forever)
    //
    err=axiom_get_fds(info->dev,&rawfd,NULL,NULL);
    if (!AXIOM_RET_IS_OK(err)) {
        elogmsg("axiom_get_fds() on recv_thread thread result=%d",err);
        zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: receiver thread end");        
        return NULL;
    }
    maxfd=rawfd>info->endfd?rawfd+1:info->endfd+1;
    for (;;) {
        FD_ZERO(&set);
        FD_SET(info->endfd,&set);
        FD_SET(rawfd,&set);
        res=select(maxfd,&set,NULL,NULL,NULL);
        if (res==-1) {
            if (errno == EINTR) continue; // paranoia
            elogmsg("select() on recv_thread thread");
            break;
        }
        if (FD_ISSET(info->endfd,&set)) {
            eventfd_t value;
            eventfd_read(info->endfd,&value); // not really needed
            zlogmsg(LOG_DEBUG, LOGZ_MASTER, "SLAVE: recv_thread received termination request");
            break;
        }                        
        // wait master messages...
        size = sizeof (buffer);
        msg = axiom_recv_raw(info->dev, &node, &port, &type, &size, &buffer);
        if (!AXIOM_RET_IS_OK(msg)) {
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: receiver thread error into axiom_recv_raw() %d", msg);
            continue;
        }
        if (logmsg_is_zenabled(LOG_TRACE, LOGZ_MASTER)) {
            if (buffer.header.command==CMD_RPC) {
                zlogmsg(LOG_TRACE, LOGZ_MASTER, "SLAVE: RECV_THREAD: received %d bytes command 0x%02x '%s' function 0x%02x '%s'",
                        size, buffer.header.command, CMD_TO_NAME(buffer.header.command),buffer.header.rpc.function,RPCFUNC_TO_NAME(buffer.header.rpc.function));
            } else {
                zlogmsg(LOG_TRACE, LOGZ_MASTER, "SLAVE: RECV_THREAD: received %d bytes command 0x%02x '%s'",
                        size, buffer.header.command, CMD_TO_NAME(buffer.header.command));
            }
        }
        if (buffer.header.command == CMD_RECV_FROM_STDIN) {
            //
            // manage stdin redirect service
            //
            if (info->services & REDIRECT_SERVICE) {
                zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: received CMD_RECV_FROM_STDIN");
                size -= sizeof (header_t);
                output(info->fd, buffer.raw, size);
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_RECV_FROM_STDIN");
            }
        } else if (buffer.header.command == CMD_KILL) {
            //
            // manage exit service
            //
            if (info->services & (EXIT_SERVICE|KILL_SERVICE)) {
                zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: received CMD_KILL");
                my_sigterm=1;
                res = kill(info->pid, info->termmode);
                zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: sent signal %d to child", info->termmode);
                if (res != 0) {
                    zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: error sending signal to controlled application errno=%d '%s'!", errno, strerror(errno));
                    if (errno!=ESRCH) {
                        /* safety! */
                        kill(info->pid, SIGKILL);
                    }
                }
                //exit(EXIT_SUCCESS);
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_KILL");
            }
        } else if (buffer.header.command == CMD_BARRIER) {
            //
            // manage barrier service
            //
            if (info->services & BARRIER_SERVICE) {
                int sz=res;
                zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: received CMD_BARRIER");
                snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), BARRIER_CHILD_TEMPLATE_NAME, (int) getpid(), buffer.header.barrier.barrier_id);
                res = sendto(sock, &buffer, sz, 0, (struct sockaddr*) &itsaddr, sizeof (itsaddr));
                if (res != sz) {
                    zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: receiver thread sendto() error (errno=%d '%s') to '%s'!", errno, strerror(errno),itsaddr.sun_path);
                }
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_BARRIER");
            }
        } else if (buffer.header.command == CMD_RPC) {
            //
            // manage RPC service
            //
            if (info->services & RPC_SERVICE) {
                zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: received CMD_RPC (func=%d '%s' id=%ld size=%d)",buffer.header.rpc.function,RPCFUNC_TO_NAME(buffer.header.rpc.function),buffer.header.rpc.id,buffer.header.rpc.size);
                snprintf(itsaddr.sun_path, sizeof (itsaddr.sun_path), RPC_CHILD_TEMPLATE_NAME, (int) getpid(), buffer.header.rpc.id);
                res = sendto(sock, &buffer, size, 0, (struct sockaddr*) &itsaddr, sizeof (itsaddr));
                if (res != sizeof (unsigned)) {
                    zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: receiver thread sendto() error (errno=%d '%s')!", errno, strerror(errno));
                }
            } else {
                zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: received an unwanted message CMD_RPC");
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
    fd_set set;
    int maxfd;
    
    // read synchornization request from a socket and send the request to the master...

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: socket thread started (thread=%ld)", (long) pthread_self());

    // socket used for slave<->child comunnication
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock == -1) {
        elogmsg("socket()");
        exit(EXIT_FAILURE);
    }
    myaddr.sun_family = AF_UNIX;
    snprintf(myaddr.sun_path, sizeof (myaddr.sun_path), SLAVE_TEMPLATE_NAME, (int) getpid());
    res = bind(sock, (struct sockaddr *) &myaddr, sizeof (myaddr));
    if (res == -1) {
        elogmsg("bind()");
        exit(EXIT_FAILURE);
    }

    //
    // MAIN LOOP
    // (forever)
    //
    maxfd=info->endfd>sock?info->endfd+1:sock+1;
    for (;;) {
        FD_ZERO(&set);
        FD_SET(sock,&set);
        FD_SET(info->endfd,&set);
        res=select(maxfd,&set,NULL,NULL,NULL);
        if (res==-1) {
            if (errno == EINTR) continue; // paranoia
            elogmsg("select() on sock_thread thread");
            break;
        }
        if (FD_ISSET(info->endfd,&set)) {
            eventfd_t value;
            eventfd_read(info->endfd,&value); // not really needed
            zlogmsg(LOG_DEBUG, LOGZ_MASTER, "SLAVE: sock_thread received termination request");
            break;
        }
        // receive data from child...
        res = recv(sock, &buffer, sizeof (buffer), MSG_WAITALL);
        if (res == -1 && errno == EAGAIN) continue;
        if (res == -1) {
            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: socket thread recv error (errno=%d '%s')", errno, strerror(errno));
            continue;
        }
        zlogmsg(LOG_TRACE, LOGZ_SLAVE, "SLAVE: SOCK_THREAD: received command 0x%02x (size=%d) from CHILD",buffer.header.command,res);
        // send request to master...
        msg = axiom_send_raw(info->dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, res, &buffer);
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
        res = kill(mypid, sig);
        if (res != 0) {
            szlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: error sending signal %d from signal handler to controlled application errno=%d '%s'!", sig, errno, strerror(errno));
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
        snprintf(sname, sizeof (sname), SLAVE_TEMPLATE_NAME, (int) getpid());
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
int manage_slave_services(axiom_dev_t *_dev, int _services, int *_fd, pid_t _pid, int termmode)
{
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
        zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: starting service threads for child pid %d",(int)_pid);

        // block all signal (so thread started have the mask set)
        block_all_signals(&oldset);

        if (_services & REDIRECT_SERVICE) {
            forout.cmd = CMD_SEND_TO_STDOUT;
            forout.fd = _fd[1];
            forout.endfd=eventfd(0,EFD_SEMAPHORE);
            if (forout.endfd==-1) {
                elogmsg("eventfd()");
                exit(EXIT_FAILURE);        
            }
            res = pthread_create(&thout, NULL, send_thread, &forout);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
            forerr.cmd = CMD_SEND_TO_STDERR;
            forerr.fd = _fd[2];
            forerr.endfd=eventfd(0,EFD_SEMAPHORE);
            if (forerr.endfd==-1) {
                elogmsg("eventfd()");
                exit(EXIT_FAILURE);        
            }
            res = pthread_create(&therr, NULL, send_thread, &forerr);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
        }
        if ((_services & (REDIRECT_SERVICE|EXIT_SERVICE|KILL_SERVICE))) {
            forin.cmd = 0;
            forin.fd = ((_services & REDIRECT_SERVICE) ? _fd[0] : -1);
            forin.endfd = eventfd(0, EFD_SEMAPHORE);
            forin.termmode = termmode;
            if (forin.endfd==-1) {
                elogmsg("eventfd()");
                exit(EXIT_FAILURE);        
            }            
            res = pthread_create(&thin, NULL, recv_thread, &forin);
            if (res != 0) {
                elogmsg("pthread_create()");
                exit(EXIT_FAILURE);
            }
        }
        if (_services & (BARRIER_SERVICE|RPC_SERVICE)) {
            forsock.endfd=eventfd(0,EFD_SEMAPHORE);
            if (forsock.endfd==-1) {
                elogmsg("eventfd()");
                exit(EXIT_FAILURE);        
            }            
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
            status=0;
            zlogmsg(LOG_TRACE, LOGZ_SLAVE, "SLAVE: entering waitpid (thread=%ld)",(long) pthread_self());
            resp = waitpid(_pid, &status, WUNTRACED|WCONTINUED);
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: waitpid result %ld", (long) resp);
            if (resp == _pid) break;
            if (resp == -1 && errno == EINTR) continue;
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "SLAVE: waitpid error res=%d '%s' errno=%d", (int) resp, strerror(errno), errno);
            break;
        }
        zlogmsg(LOG_DEBUG,LOGZ_SLAVE,"SLAVE: waitpid status=0x%08x exited=%d signaled=%d continued=%d stopped=%d exit=%d signal=%d",status,WIFEXITED(status),WIFSIGNALED(status),WIFCONTINUED(status),WIFCONTINUED(status),WEXITSTATUS(status),WTERMSIG(status));
        if (my_sigterm) {
            // NB: there may be a critical run on SIGTERM signal (i.e. it is generated by the child or the slave has send it?)
            if (WIFSIGNALED(status) && WTERMSIG(status) == termmode) {
                status=0;
                zlogmsg(LOG_DEBUG,LOGZ_SLAVE,"SLAVE: status reset to 0");
            }
        }
    }

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: child process %s...", _pid > 0 ? "died" : "never started");

    if (_services && _pid > 0) {
        //
        // end service threads...
        //
        zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: waiting working threads...");
        if (_services & REDIRECT_SERVICE) {
            terminate_thread_slave(thout,forout.endfd);
            close(forout.endfd);
            terminate_thread_slave(therr,forerr.endfd);
            close(forerr.endfd);
        }
        if (_services & (REDIRECT_SERVICE|EXIT_SERVICE|KILL_SERVICE)) {
            terminate_thread_slave(thin,forin.endfd);
            close(forin.endfd);
        }
        if (_services & (BARRIER_SERVICE|RPC_SERVICE)) {
            char sname[UNIX_PATH_MAX];
            terminate_thread_slave(thsock,forsock.endfd);
            close(forsock.endfd);
            snprintf(sname, sizeof (sname), SLAVE_TEMPLATE_NAME, (int) getpid());
            unlink(sname);
        }
        zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: working threads died");

        if (_services & REDIRECT_SERVICE) {
            struct timeval tv;
            fd_set set;
            buffer_t buffer;
            int nfds, ret, sz, closedout, closederr;
            axiom_msg_id_t msg;

            zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: waiting spurious stdout/stderr data");
            tv.tv_sec = 0;
            tv.tv_usec = 750000;
            closedout = closederr = 0;
            for (;;) {
                FD_ZERO(&set);
                nfds = 0;
                if (!closedout) {
                    FD_SET(_fd[1], &set);
                    nfds = _fd[1];
                }
                if (!closederr) {
                    FD_SET(_fd[2], &set);
                    nfds = nfds > _fd[2] ? nfds : _fd[2];
                }
                ret = select(nfds + 1, &set, NULL, NULL, &tv);
                // NMB: we assume, as linux do but posix permits not to do, that struct timeval will be updated
                if (ret == -1) {
                    elogmsg("SLAVE: select()");
                    break;
                }
                zlogmsg(LOG_TRACE, LOGZ_SLAVE, "SLAVE: remaining %ld usec (for spurios write)", tv.tv_usec);
                if (!ret) break;
                if (FD_ISSET(_fd[1], &set)) {
                    sz = read(_fd[1], buffer.raw, sizeof (buffer.raw));
                    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: STDOUT read %d bytes from fd", (int) sz);
                    if (sz == -1) {
                        zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: STDOUT read error (errno=%d '%s')", errno, strerror(errno));
                    } else if (sz > 0) {
                        buffer.header.command = CMD_SEND_TO_STDOUT;
                        msg = axiom_send_raw(_dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sz + sizeof (header_t), &buffer);
                        if (!AXIOM_RET_IS_OK(msg))
                            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: STDOUT axiom_send_raw() write error (err=%d)", msg);
                    } else {
                        closedout = 1;
                    }
                }
                if (FD_ISSET(_fd[2], &set)) {
                    sz = read(_fd[2], buffer.raw, sizeof (buffer.raw));
                    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: STDERR read %d bytes from fd", (int) sz);
                    if (sz == -1) {
                        zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: STDERR read error (errno=%d '%s')", errno, strerror(errno));
                    } else if (sz > 0) {
                        buffer.header.command = CMD_SEND_TO_STDERR;
                        msg = axiom_send_raw(_dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sz + sizeof (header_t), &buffer);
                        if (!AXIOM_RET_IS_OK(msg))
                            zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: STDERR axiom_send_raw() write error (err=%d)", msg);
                    } else {
                        closederr = 1;
                    }
                }
                if (closedout && closederr) break;
            }
        }

    }

    zlogmsg(LOG_INFO, LOGZ_SLAVE, "SLAVE: send CMD_EXIT message to master with status 0x%08x",status);

    //
    // send master child termination notification...
    //
    buffer.header.command = CMD_EXIT;
    buffer.header.status = status;
    msgid = axiom_send_raw(_dev, master_node, master_port, AXIOM_TYPE_RAW_DATA, sizeof (header_t), &buffer);
    if (!AXIOM_RET_IS_OK(msgid)) {
        zlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: sending CMD_EXIT to master errror res=%d", msgid);
    }
    
    return status;
}
