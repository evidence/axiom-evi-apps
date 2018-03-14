/*!
 * \file common.h
 *
 * \version     v1.2
 *
 * Used internally by axiom-run.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

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
/** command rpc (master<->slave) */
#define CMD_RPC             0x86
/** command START (master->slave) */
#define CMD_START           0x87

/** template name for the master unix domani socket port */
#define SLAVE_TEMPLATE_NAME "/tmp/ax%d"
/** template name for the slave unix domain socket port */
#define BARRIER_CHILD_TEMPLATE_NAME "/tmp/axbar%d.%d"
/** template name for the slave unix domain socket port */
#define RPC_CHILD_TEMPLATE_NAME "/tmp/axrpc%d.%ld"

/**
 *      * header of all raws message between master and slave
 *      */
typedef struct {
    /** command. see CMD_??? defines */
    uint8_t command;
    /** dont't care */
    uint8_t pad[3];

    union {
        /** exit status. used only by CMD_EXIT messages */
        int status;

        /** barried id. used only by CMD_BARRIER messages */
        struct {
            unsigned barrier_id;
        } barrier;

        /** rpc data. used only for CMD_RPC message */
        struct {
            uint64_t id;
            uint32_t function;
            uint32_t size;
        } rpc;
        /** magic. used for the CMD_START initial synchronization */
        uint64_t magic;
    } __attribute__((__packed__));

} __attribute__((__packed__)) header_t;


#endif /* COMMON_H */

