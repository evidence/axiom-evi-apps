/**
 * Used internally by axiom-run.
 * 
 * @file   axiom-run.h
 * @version v0.7
 */

#ifndef AXIOM_RUN_H
#define AXIOM_RUN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
    
#include "axiom_nic_types.h"
#include "axiom_nic_limits.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_regs.h"
#include "axiom_nic_packets.h"
#include "axiom_init_api.h"
#include "axiom_common.h"

    /* message log zones */

    /** main zone */
#define LOGZ_MAIN   0x01
    /** master zone */
#define LOGZ_MASTER 0x02
    /** slave zone */
#define LOGZ_SLAVE  0x04
    /** catch all zone */
#define LOGZ_ALL    0x07

    /** max number of nodes supported */
#define MAX_NUM_NODES 64
    /** max output buffer size */
#define MAX_BUFFER_SIZE BUFSIZ

    /** generic default port number. used for comunication between maste and slave */
#define MY_DEFAULT_PORT 5
    /** default master port used */
#define MY_DEFAULT_MASTER_PORT  (MY_DEFAULT_PORT)
    /** default slave port used */
#define MY_DEFAULT_SLAVE_PORT  (MY_DEFAULT_PORT+1)
    /** default master node used */
#define MY_DEFAULT_MASTER_NODE 0

    /* master/slave commands */
    /** command exit (slave->master) */
#define CMD_EXIT            0x80
    /** command kill (master->slave) */
#define CMD_KILL            0x81
    /** command to redirect stdout (slave->master) */
#define CMD_SEND_TO_STDOUT  0x82
    /** command to redirect stderr (slave->master) */
#define CMD_SEND_TO_STDERR  0x83
    /** command to redirect stdin (master->slave) */
#define CMD_RECV_FROM_STDIN 0x84
    /** command barrier (master<->slave) */
#define CMD_BARRIER         0x85

    /**
     * header of all raws message between master ans slave
     */
    typedef struct {
        /** command. see CMD_??? deines */
        uint8_t command;
        /** dont't care */
        uint8_t pad[3];

        union {
            /** exit status. used only by CMD_EXIT messages */
            int status;
            /** barried id. used only by CMD_BARRIER messages */
            unsigned barrier_id;
        } __attribute__((__packed__));
        
    } __attribute__((__packed__)) header_t;

    /**
     * message between master and slave structure
     */
    typedef struct {
        /** header */
        header_t header;
        /** raw data. the semantic depend by the command message field */
        uint8_t raw[AXIOM_RAW_PAYLOAD_MAX_SIZE - sizeof (header_t)];
    } __attribute__((__packed__)) buffer_t;

    /**
     * Block all signal.
     * @param oldset where to save the old signal mask (if not null)
     */
    void block_all_signals(sigset_t *oldset);

    /**
     * Restore all signal and set quit signal handler.
     * @param oldset if not null is used to restore old signal mask
     * @param myhandler if not null is installed for various quit signal
     */
    void restore_signals_and_set_quit_handler(sigset_t *oldset, void (*myhandler)(int));

    /**
     * Terminate a pthread thread.
     * @param th the thread to terminate
     * @param logheader a header to prefix all message log (can't be null)
     */
    void terminate_thread(pthread_t th, char *logheader);

    /**
     * Terminate a thread from slave process.
     * Use terminate_thread().
     * @param th the thread to terminate
     */
#define terminate_thread_slave(th) terminate_thread(th,"SLAVE");
    /**
     * Terminate a thread from master process.
     * Use terminate_thread().
     * @param th the thread to terminate
     */
#define terminate_thread_master(th) terminate_thread(th,"MASTER");

    /* service bitwise */

    /** barrier service*/
#define BARRIER_SERVICE 0x01
    /** exit service */
#define EXIT_SERVICE 0x02
    /* redirect service */
#define REDIRECT_SERVICE 0x04

    /* flags */

    /** ident flag. the master emit a message to identify wich node write a message (used by redirect service). */
#define IDENT_FLAG 0x01
    /** the form of identification. if IDENT_FLAG is set two form of identification can be written. */
#define ALTERNATE_IDENT_FLAG 0x02

    /**
     * Run and manage services for master process.
     * @param dev axiom device for communication
     * @param services services bitwise
     * @param nodes nodes bitwise
     * @param flags flags
     */
    void manage_master_services(axiom_dev_t *dev, int services, uint64_t nodes, int flags);

    /**
     * Run and manage services for slave process.
     * @param dev axiom device for communication
     * @param services services bitwise
     * @param fd array of 3 file descriptor for redirect service (if enabled)
     * @param pid process id of child process (application controlled)
     */
    void manage_slave_services(axiom_dev_t *dev, int services, int *fd, pid_t pid);

    /** node number of the master  */
    extern int master_node;
    /** port number of the master */
    extern int master_port;
    /** port number of the slave */
    extern int slave_port;
    
#ifdef __cplusplus
}
#endif

#endif /* AXIOM_RUN_H */

