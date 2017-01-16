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
#include "axiom_nic_init.h"
#include "axiom_init_api.h"
#include "axiom_common.h"
#include "axiom_run_api.h"
#include "axiom_allocator_protocol.h"
#include "axiom_allocator_l2.h"

#include "common.h"
    
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

    extern char *cmd_to_name[];
#define CMD_TO_NAME(cmd) ((cmd)>=CMD_EXIT&&(cmd)<=CMD_RPC?cmd_to_name[(cmd)-CMD_EXIT]:"unknown")
    extern char *rpcfunc_to_name[];
#define RPCFUNC_TO_NAME(func) ((func)>=AXRUN_RPC_PING&&(func)<=AXRUN_RPC_PING?rpcfunc_to_name[(func)-AXRUN_RPC_PING]:"unknown")


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
    void terminate_thread(pthread_t th, int endfd, char *logheader);

    /**
     * Terminate a thread from slave process.
     * Use terminate_thread().
     * @param th the thread to terminate
     */
#define terminate_thread_slave(th, endfd) terminate_thread(th, endfd, "SLAVE");
    /**
     * Terminate a thread from master process.
     * Use terminate_thread().
     * @param th the thread to terminate
     */
#define terminate_thread_master(th, endfd) terminate_thread(th, endfd, "MASTER");

    /* service bitwise */

    /** barrier service*/
#define BARRIER_SERVICE 0x01
    /** exit service */
#define EXIT_SERVICE 0x02
    /* redirect service */
#define REDIRECT_SERVICE 0x04
    /** rpc service*/
#define RPC_SERVICE 0x08
    /* allocator service */
#define ALLOCATOR_SERVICE 0x10
    /** kill service */
#define KILL_SERVICE 0x20

    /* flags */

    /** ident flag. the master emit a message to identify wich node write a message (used by redirect service). */
#define IDENT_FLAG 0x01
    /** the form of identification. if IDENT_FLAG is set two form of identification can be written. */
#define ALTERNATE_IDENT_FLAG 0x02

#define EXIT_FLAG_MASK 0x1c
#define EXIT_FLAG_SHIFT 2
#define FIRST_ABS_EXIT_FLAG (0x5<<EXIT_FLAG_SHIFT)
#define NOFAIL_EXIT_FLAG (0x4<<EXIT_FLAG_SHIFT)
#define FIRST_EXIT_FLAG (0x03<<EXIT_FLAG_SHIFT)
#define LAST_EXIT_FLAG (0x02<<EXIT_FLAG_SHIFT)
#define LESSER_EXIT_FLAG (0x01<<EXIT_FLAG_SHIFT)
#define GREATHER_EXIT_FLAG (0x00<<EXIT_FLAG_SHIFT)
#define NORMAL_EXIT_FLAG GREATHER_EXIT_FLAG

    /*
     * Run and manage services for master process.
     * @param dev axiom device for communication
     * @param services services bitwise
     * @param nodes nodes bitwise
     * @param flags flags
     * @return exit status (see 'man 2 waitpid')
     */
    int manage_master_services(axiom_dev_t *dev, int services, uint64_t nodes, int flags, axiom_app_id_t app_id);

    /**
     * Run and manage services for slave process.
     * @param dev axiom device for communication
     * @param services services bitwise
     * @param fd array of 3 file descriptor for redirect service (if enabled)
     * @param pid process id of child process (application controlled)
     * @param termmode signal usde to kill child process
     * @return exit status (see 'man 2 waitpid')
     */
    int manage_slave_services(axiom_dev_t *dev, int services, int *fd, pid_t pid, int termode);

    /** node number of the master  */
    extern int master_node;
    /** port number of the master */
    extern int master_port;
    /** port number of the slave */
    extern int slave_port;

    int rpc_init(axiom_app_id_t app_id);
    int rpc_service(axiom_dev_t *dev, axiom_node_id_t src_node, size_t size, buffer_t *inmsg);
    void rpc_release(axiom_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* AXIOM_RUN_H */

