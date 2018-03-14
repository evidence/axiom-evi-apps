/*!
 * \file axiom_session.c
 *
 * \version     v1.2
 *
 * Axiom session management.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "../axiom-init.h"

/** Redifinition on __builin_ctz() */
#define __ctz(x) __builtin_ctz(x)

/** Sessions.
 * Bitwise: 1 free 0 used.
 * Session x is free if bit x in 1.
 */
static unsigned int sessions = UINT_MAX;

/**
 * Find the next session (unused).
 * @return the next session number (if available).
 */
static __inline uint8_t next() {
    register uint8_t ses = AXIOM_SESSION_EMPTY;
    if (sessions != 0) {
        ses = __ctz(sessions);
        sessions &= ~(1 << ses);
    }
    return ses;
}

/**
 * Release (free) a session.
 * @param ses A session id to free.
 */
static __inline void release(uint8_t ses) {
    assert(ses >= 0 && ses<sizeof (unsigned int)*8);
    sessions |= (1 << (ses));
}

/**
 * Test if a session id is used.
 * @param ses The session to test.
 * @return 1 is used 0 otherwise.
 */
static __inline int used(uint8_t ses) {
    return ses>sizeof (unsigned int)*8 ? 0 : ((sessions & (1 << ses)) == 0);
}

/**
 * Release a session.
 * @param ses the session to release.
 */
void axiom_session_release(uint8_t ses) {
    release(ses);
}

/**
 * Test if a session is already used,
 * @param ses The session to test.
 * @return T1 if used 0 otherwise.
 */
int axiom_session_is_used(uint8_t ses) {
    return used(ses);
}

/**
 * Manage a axiom request session message.
 *
 * @param dev The axiom device.
 * @param src The source node.
 * @param payload_size The size of the payload.
 * @param _payload the payload.
 * @param verbose Emit verbose messages.
 */
void axiom_session(axiom_dev_t *dev, axiom_node_id_t src, size_t payload_size, void *_payload, int verbose) {
    axiom_session_req_payload_t *payload =
        ((axiom_session_req_payload_t *) _payload);
    axiom_session_reply_payload_t *payload2 =
        ((axiom_session_reply_payload_t *) _payload);
    axiom_port_t port;
    axiom_err_t ret;

    if (payload->command != AXIOM_CMD_SESSION_REQ) {
        EPRINTF("SESSION - received not AXIOM_SESSION_REQ message");
        return;
    }
    port = payload->reply_port;
    IPRINTF(verbose, "SESSION - REQ message received - src_node: %u reply_port: %u", src, port);

    if (payload->session_id != AXIOM_SESSION_EMPTY) {
        //
        // release
        //
        if (used(payload->session_id)) {
            release(payload->session_id);
            IPRINTF(verbose, "SESSION - REQ message - release - session: %u", payload->session_id);
        }
    } else {
        //
        // alloc
        //
        payload2->command = AXIOM_CMD_SESSION_REPLY;
        payload2->session_id = next();

        ret = axiom_send_raw(dev, src, port, AXIOM_TYPE_RAW_DATA, sizeof(*payload2), payload2);
        if (!AXIOM_RET_IS_OK(ret)) {
            EPRINTF("ERROR - send small message to node %u error", src);
        }
        IPRINTF(verbose, "SESSION - REQ message - acquire - session: %u", payload2->session_id);
        //IPRINTF(verbose, "SESSION - REQ message dump - size: %lu dump: 0x%02x 0x%02x 0x%02x 0x%02x", sizeof(*payload2),*(((uint8_t*)payload2)+0) ,*(((uint8_t*)payload2)+1), *(((uint8_t*)payload2)+2), *(((uint8_t*)payload2)+3));
    } 
}
