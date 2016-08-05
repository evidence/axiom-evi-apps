/**
 * Manage services for axion-run master process.
 *
 * @file master.c
 * @version v0.7
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <assert.h>

#include "axiom-run.h"
#include "axiom_run_api.h"

/**
 * Some information for the threads.
 */
typedef struct {
    /** axiom device */
    axiom_dev_t *dev;
    /** service bitwise */
    int services;
    /** nodes bitwise */
    uint64_t nodes;
    /** number of nodes */
    int nnodes;
    /** running flags. see axiom-run.h */
    int flags;
} thread_info_t;

/**
 * Send the same axiom raw message to all nodes.
 *
 * @param dev axiom device
 * @param nodes nodes bitwise
 * @param port receiver port
 * @param size size of the payload
 * @param payload the payload
 * @return AXIOM_RET_OK in case of succes
 */
static axiom_err_t my_axiom_send_raw(axiom_dev_t *dev, uint64_t nodes, axiom_port_t port, axiom_raw_payload_size_t size, axiom_payload_t *payload) {
    axiom_node_id_t node = 0;
    axiom_err_t err = AXIOM_RET_OK;
    axiom_msg_id_t msg;
    while (nodes != 0) {
        if (nodes & 0x1) {
            msg = axiom_send_raw(dev, node, port, AXIOM_TYPE_RAW_DATA, size, payload);
            if (!AXIOM_RET_IS_OK(msg)) {
                err = AXIOM_RET_ERROR;
                zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: axiom_send_raw() error %d while sending mesaage to node %d", msg, node);
            }
        }
        node++;
        nodes >>= 1;
    }
    return err;
}

/**
 * Send the same axiom raw message to all nodes using safe log message.
 * Is the same as my_axiom_send_raw but use signal safe handler log message.
 * So it is safe to use into a signal handeler.
 * It is used only if an exit signal is received to the master to send a CMD_KILL to all the slaves.
 * 
 * @param dev axiom device
 * @param nodes nodes bitwise
 * @param port receiver port
 * @param size size of the payload
 * @param payload the payload
 * @return AXIOM_RET_OK in case of succes
 */
static axiom_err_t s_my_axiom_send_raw(axiom_dev_t *dev, uint64_t nodes, axiom_port_t port, axiom_raw_payload_size_t size, axiom_payload_t *payload) {
    //
    // :-((((
    //
    axiom_node_id_t node = 0;
    axiom_err_t err = AXIOM_RET_OK;
    axiom_msg_id_t msg;
    while (nodes != 0) {
        if (nodes & 0x1) {
            msg = axiom_send_raw(dev, node, port, AXIOM_TYPE_RAW_DATA, size, payload);
            if (!AXIOM_RET_IS_OK(msg)) {
                err = AXIOM_RET_ERROR;
                szlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: axiom_send_raw() error %d while sending mesaage to node %d", msg, node);
            }
        }
        node++;
        nodes >>= 1;
    }
    return err;
}

/** output buffer for pending output */
typedef struct {
    /** buffer of pending output */
    uint8_t *buffer;
    /** number of bytes used into the buffer */
    int sz;
} output_info_t;

/** barrier info */
typedef struct {
    /* number of nodes that need to reach the barrier*/
    int counter;
} barrier_info_t;

/**
 * Write a buffer to a file.
 * Usually stdout or stderr.
 *
 * @param ptr cahracters buffer
 * @param size size of the buffer
 * @param fout write to this file
 */
static void output(uint8_t *ptr, int size, FILE *fout) {
    int sz;
    while (size > 0) {
        sz = fwrite(ptr, sizeof (uint8_t), size, fout);
        if (sz == 0) break;
        size -= sz;
        ptr += sz;
    }
}

/**
 * Flush pending buffer.
 * Add a line break after every pending line.
 * 
 * @param info array of "output pending" information
 * @param num_nodes number of nodes
 * @param fout write to this file.
 * @param flags flags. see axiom-run.h
 */
static void flush(output_info_t *info, int num_nodes, FILE *fout, int flags) {
    int node;
    for (node = 0; node < num_nodes; node++) {
        if (info[node].sz > 0) {
            if (flags && IDENT_FLAG) fprintf(fout, flags & ALTERNATE_IDENT_FLAG ? "{%02d}" : "%02d:", node);
            output(info[node].buffer, info[node].sz, fout);
            info[node].sz = 0;
            fputc('\n', fout);
        }
    }
    fflush(fout);
}

/**
 * Write a buffer to stdout/stderr line buffered.
 * Emit only full lines. The partial line are append to the "output pending" buffer.
 * If the size of the "output pending" buffer is greather than MAX_BUFFER_SIZE then the line is emitted (not line break terminated!).
 * 
 * @param node the source node. the buffer data came from this node
 * @param buffer the buffer
 * @param size the size of the buffer
 * @param info the "output pending" information for the node
 * @param fout write to this file
 * @param flags flags. see axiom-run.h
 */
static void emit(axiom_node_id_t node, uint8_t *buffer, int size, output_info_t *info, FILE *fout, int flags) {
    const int emit_id = flags&IDENT_FLAG;
    const int alternate = flags&ALTERNATE_IDENT_FLAG;
    int no_stop_emit = 1;
    int res;
    uint8_t *p;

    assert(node >= 0 && node < MAX_NUM_NODES);
    p = (emit_id ? memchr(buffer, '\n', size) : memrchr(buffer, '\n', size));
    if (p != NULL) {
        if (info[node].sz > 0) {
            if (emit_id) fprintf(fout, alternate ? "{%02d}" : "%02d:", node);
            output(info[node].buffer, info[node].sz, fout);
            info[node].sz = 0;
            no_stop_emit = 0;
        }
        for (;;) {
            res = p - buffer + 1;
            if (emit_id && no_stop_emit) fprintf(fout, alternate ? "{%02d}" : "%02d:", node);
            no_stop_emit = 1;
            output(buffer, res, fout);
            buffer += res;
            size -= res;
            p = memchr(buffer, '\n', size);
            if (p == NULL) break;
        }
        if (size != 0) {
            info[node].sz = size;
            assert(info[node].sz <= MAX_BUFFER_SIZE);
            memcpy(info[node].buffer, buffer, info[node].sz);
        }
        fflush(fout);
    } else {
        if (info[node].sz + size > MAX_BUFFER_SIZE) {
            if (info[node].sz > 0) {
                output(info[node].buffer, info[node].sz, fout);
                info[node].sz = 0;
            }
        }
        assert(size + info[node].sz <= MAX_BUFFER_SIZE);
        memcpy(info[node].buffer + info[node].sz, buffer, size);
        info[node].sz += size;
    }
}

/**
 * Thread that receive messaged from slaves.
 *
 * @param data information required to the thread
 * @return don't care
 */
static void *master_receiver(void *data) {
    thread_info_t *info = (thread_info_t*) data;
    axiom_node_id_t node;
    axiom_port_t port;
    axiom_type_t type;
    buffer_t buffer;
    axiom_msg_id_t msg;
    axiom_raw_payload_size_t size;
    int i;
    int exit_counter = info->nnodes;
    output_info_t *infoout, *infoerr;
    barrier_info_t *barrier;

    //
    // initialization
    //
    infoout = malloc(sizeof (output_info_t) * MAX_NUM_NODES);
    lassert(infoout != NULL);
    infoerr = malloc(sizeof (output_info_t) * MAX_NUM_NODES);
    lassert(infoerr != NULL);
    for (i = 0; i < MAX_NUM_NODES; i++) {
        infoout[i].sz = 0;
        infoout[i].buffer = malloc(MAX_BUFFER_SIZE);
        lassert(infoout[i].buffer != NULL);
        infoerr[i].sz = 0;
        infoerr[i].buffer = malloc(MAX_BUFFER_SIZE);
        lassert(infoerr[i].buffer != NULL);
    }
    barrier = malloc(sizeof (barrier_info_t)*(AXRUN_MAX_BARRIER_ID + 1));
    lassert(barrier != NULL);
    memset(barrier, 0, sizeof (barrier_info_t)*(AXRUN_MAX_BARRIER_ID + 1));

    //
    // thread MAIN LOOP
    //
    // note that we exit from this loop when we have received a EXIT message from all child
    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: entering receiver thread");
    for (;;) {
        size = sizeof (buffer);
        //
        // waiting for slave message...
        //
        msg = axiom_recv_raw(info->dev, &node, &port, &type, &size, &buffer);
        if (!AXIOM_RET_IS_OK(msg)) {
            zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: axiom_recv_raw() error %d", msg);
            continue;
        }
        logmsg(LOG_TRACE, "MASTER: received %d bytes command=%d", size, buffer.header.command);
        if (buffer.header.command == CMD_SEND_TO_STDERR) {
            //
            // redirect stderr service...
            //
            if (info->services & REDIRECT_SERVICE) {
                size -= sizeof (header_t);
                emit(node, buffer.raw, size, infoerr, stderr, info->flags);
            } else {
                zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: received not served CMD_SET_TO_STDOUT message from node %d", node);
            }
        } else if (buffer.header.command == CMD_SEND_TO_STDOUT) {
            //
            // redirect stdout service...
            //
            if (info->services & REDIRECT_SERVICE) {
                size -= sizeof (header_t);
                emit(node, buffer.raw, size, infoout, stdout, info->flags);
            } else {
                zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: received not served CMD_SEND_TO_STDOUT message from node %d", node);
            }
        } else if (buffer.header.command == CMD_EXIT) {
            //
            // exit service/information...
            //
            zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: received EXIT message from %d", node);
            if (info->services & EXIT_SERVICE) {
                zlogmsg(LOG_DEBUG, LOGZ_MASTER, "MASTER: send CMD_EXIT to all");
                my_axiom_send_raw(info->dev, info->nodes, slave_port, sizeof (header_t), (axiom_payload_t *) & buffer);
                //exit(EXIT_SUCCESS);
                exit_counter--;
                zlogmsg(LOG_DEBUG, LOGZ_MASTER, "exit_counter now is %d", exit_counter);
                info->services &= ~EXIT_SERVICE;
            } else {
                exit_counter--;
                zlogmsg(LOG_DEBUG, LOGZ_MASTER, "exit_counter now is %d", exit_counter);
                if (exit_counter == 0) {
                    zlogmsg(LOG_DEBUG, LOGZ_MASTER, "MASTER: exit_counter reach zero... exiting...");
                    //exit(EXIT_SUCCESS);
                    break;
                }
            }
        } else if (buffer.header.command == CMD_BARRIER) {
            //
            // barrier service...
            //
            zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: received BARRIER message from %d", node);
            if (info->services & BARRIER_SERVICE) {
                if (buffer.header.barrier_id <= AXRUN_MAX_BARRIER_ID) {
                    unsigned id = buffer.header.barrier_id;
                    if (barrier[id].counter == 0) {
                        barrier[id].counter = info->nnodes;
                    }
                    barrier[id].counter--;
                    if (barrier[id].counter == 0) {
                        // SEND SYNC TO SLAVES
                        my_axiom_send_raw(info->dev, info->nodes, slave_port, sizeof (header_t), (axiom_payload_t*) & buffer);
                    }
                } else {
                    zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: BARRIER message from node %d with id=%d out of bound", node, buffer.header.barrier_id);
                }
            } else {
                zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: received not served BARRIER message from node %d", node);
            }
        } else {
            zlogmsg(LOG_ERROR, LOGZ_MASTER, "unknown message command 0x%02x", buffer.header.command);
        }
    }
    flush(infoout, MAX_NUM_NODES, stdout, info->flags);
    flush(infoerr, MAX_NUM_NODES, stderr, info->flags);
    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: exiting receiver thread");

    // release resources
    free(barrier);
    for (i = 0; i < MAX_NUM_NODES; i++) {
        free(infoout[i].buffer);
        free(infoerr[i].buffer);
    }
    free(infoout);
    free(infoerr);

    return NULL;
}

/**
 * Thread that receive stdin data.
 *
 * @param data information required to the thread
 * @return don't care
 */
static void *master_sender(void *data) {
    thread_info_t *info = (thread_info_t*) data;
    buffer_t buffer;
    size_t sz;

    //
    // read stdin and send this data to all slaves...
    //

    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: entering redirect loop for STDIN");
    buffer.header.command = CMD_RECV_FROM_STDIN;
    //
    // main loop
    // forever...
    for (;;) {
        sz = read(STDIN_FILENO, buffer.raw, sizeof (buffer.raw));
        if (sz == -1) {
            if (errno == EINTR) continue;
            zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: read() failure (errno=%d '%s')", errno, strerror(errno));
            break;
        }
        if (sz > 0) {
            my_axiom_send_raw(info->dev, info->nodes, slave_port, sz + sizeof (header_t), (axiom_payload_t *) & buffer);
        }
    }
    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: exiting redirect loop for STDIN");
    return NULL;
}

static axiom_dev_t *mydev;
static uint64_t mynodes;

/**
 * Exit signal handler.
 * Send messgae to alla slave to terminate application.
 * @param sig
 */
static void myexit(int sig) {
    buffer_t buffer;
    szlogmsg(LOG_INFO, LOGZ_SLAVE, "MASTER: caught exit signal... exiting...");
    if (mydev != NULL) {
        buffer.header.command = CMD_EXIT;
        buffer.header.status = sig & 0x7f; // HACK: works... for now.... see <bits/waitstatus.h>
        s_my_axiom_send_raw(mydev, mynodes, slave_port, sizeof (header_t), (axiom_payload_t *) & buffer);
    } else {
        szlogmsg(LOG_WARN, LOGZ_SLAVE, "SLAVE: mydev is NULL!!!!");
    }
    exit(EXIT_SUCCESS);
}

/* see axiom-run.h */
void manage_master_services(axiom_dev_t *_dev, int _services, uint64_t _nodes, int _flags) {
    pthread_t threcv, thsend;
    thread_info_t recvinfo, sendinfo;
    sigset_t oldset;
    int nnodes;
    uint64_t n;
    int res;

    /* count the number of slaves nodes */
    nnodes = 0;
    n = _nodes;
    while (n != 0) {
        if (n & 0x1) nnodes++;
        n >>= 1;
    }

    recvinfo.nnodes = sendinfo.nnodes = nnodes;
    recvinfo.dev = sendinfo.dev = _dev;
    recvinfo.nodes = sendinfo.nodes = _nodes;
    recvinfo.services = sendinfo.services = _services;
    recvinfo.flags = sendinfo.flags = _flags;

    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: starting service threads");

    //
    // create service threads....
    //

    block_all_signals(&oldset);

    res = pthread_create(&threcv, NULL, master_receiver, &recvinfo);
    if (res != 0) {
        elogmsg("pthread_create()");
        exit(EXIT_FAILURE);
    }
    if (_services & REDIRECT_SERVICE) {
        res = pthread_create(&thsend, NULL, master_sender, &sendinfo);
        if (res != 0) {
            elogmsg("pthread_create()");
            exit(EXIT_FAILURE);
        }
    }

    mydev = _dev;
    mynodes = _nodes;
    restore_signals_and_set_quit_handler(&oldset, myexit);

    //
    // wait recevier thread
    // (and kill remainig threads)
    //
    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: waiting service threads");

    res = pthread_join(threcv, NULL);
    if (res != 0) {
        elogmsg("pthread_join()");
        exit(EXIT_FAILURE);
    }
    if (_services & REDIRECT_SERVICE) {
        terminate_thread_master(thsend);
    }

    zlogmsg(LOG_INFO, LOGZ_MASTER, "MASTER: all service threads are dead");
}