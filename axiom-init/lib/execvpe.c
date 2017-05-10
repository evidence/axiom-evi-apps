/*!
 * \file execvpe.c
 *
 * \version     v0.12
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "axiom_nic_raw_commands.h"
#include "axiom_nic_limits.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"

#include "axiom_common.h"
#include "axiom_init_api.h"

int axinit_execvpe(axiom_dev_t *dev, axiom_port_t port, axiom_node_id_t node, int flags, const char *filename, char *const argv[], char *const envp[]) {
    uint8_t packet_buffer[AXIOM_SPAWN_MAX_SIZE];
    axiom_msg_id_t _msg;
    axiom_node_id_t _node;
    axiom_port_t _port;
    axiom_type_t _type;
    axiom_raw_payload_size_t _size;
    axiom_session_req_payload_t *payload_session_req = (axiom_session_req_payload_t*) packet_buffer;
    axiom_session_reply_payload_t *payload_session_reply = (axiom_session_reply_payload_t*)packet_buffer;
    axiom_spawn_req_payload_t *payload_spawn_request = (axiom_spawn_req_payload_t*) packet_buffer;
    uint8_t session = AXIOM_SESSION_EMPTY;
    int len;
    char *s;
    int broadcast = (flags & AXINIT_EXEC_BROADCAST);
    int start_counter = 0;

    logmsg(LOG_DEBUG, "axinit_execvpe: start");

    // for broadcast :-(
    if (broadcast) {
        logmsg(LOG_DEBUG, "axinit_execvpe: broadcast mode");
        node = 0;
    }

    do {
        if ((flags & AXINIT_EXEC_NOSELF) && node == axiom_get_node_id(dev)) continue;

        payload_session_req->command = AXIOM_CMD_SESSION_REQ;
        payload_session_req->reply_port = port;
        payload_session_req->session_id = AXIOM_SESSION_EMPTY;
        logmsg(LOG_DEBUG, "axinit_execvpe: send session request message to node %d port %u", node, AXIOM_RAW_PORT_INIT);
        _msg = axiom_send_raw(dev, node, AXIOM_RAW_PORT_INIT, AXIOM_TYPE_RAW_DATA, sizeof (*payload_session_req), payload_session_req);
        if (!AXIOM_RET_IS_OK(_msg)) return _msg;

        _size = sizeof (axiom_init_payload_t);
        logmsg(LOG_DEBUG, "axinit_execvpe: waiting replay");
        _msg = axiom_recv_raw(dev, &_node, &_port, &_type, &_size, &packet_buffer);
        if (!AXIOM_RET_IS_OK(_msg)) return _msg;
        session = payload_session_reply->session_id;
        if (session == AXIOM_SESSION_EMPTY) return AXIOM_RET_ERROR;
        logmsg(LOG_DEBUG, "axinit_execvpe: session acquired %d",session);
        logmsg(LOG_TRACE, "axinit_execvpe: session reply DUMP size_recv: %lu size_buf: %lu dump: 0x%02x 0x%02x 0x%02x 0x%02x", (unsigned long)_size, sizeof(*payload_session_reply),*(((uint8_t*)payload_session_reply)+0) ,*(((uint8_t*)payload_session_reply)+1), *(((uint8_t*)payload_session_reply)+2), *(((uint8_t*)payload_session_reply)+3));

        payload_spawn_request->command = AXIOM_CMD_SPAWN_REQ;
        payload_spawn_request->flags = AXIOM_SPAWN_FLAG_RESET;
        payload_spawn_request->session_id = session;
        payload_spawn_request->type = AXIOM_SPAWN_TYPE_EXE;
        len = strlcpy((char*) payload_spawn_request->data, filename, sizeof (payload_spawn_request->data));

        logmsg(LOG_DEBUG, "axinit_execvpe: send EXEC spawn message '%s'", (char*) payload_spawn_request->data);
        _msg = axiom_send_raw(dev, node, AXIOM_RAW_PORT_INIT, AXIOM_TYPE_RAW_DATA, AXIOM_SPAWN_HEADER_SIZE + len + 1, payload_spawn_request);
        if (!AXIOM_RET_IS_OK(_msg)) goto release;

        if (argv != NULL) {
            char * const *pargs = argv;
            payload_spawn_request->flags = 0;
            payload_spawn_request->type = AXIOM_SPAWN_TYPE_ARG;
            while (*pargs != NULL) {
                len = strlcpy((char*) payload_spawn_request->data, *pargs, sizeof (payload_spawn_request->data));
                logmsg(LOG_DEBUG, "axinit_execvpe: send ARG spawn message len=%d '%s'", len, (char*) payload_spawn_request->data);
                if (len+1>=AXIOM_SPAWN_MAX_DATA_SIZE) {
                    logmsg(LOG_WARN, "axinit_execvpe: send ARG spawn message TRUNCATED! size>%d",AXIOM_SPAWN_MAX_DATA_SIZE);
                }
                _msg = axiom_send(dev, node, AXIOM_RAW_PORT_INIT, AXIOM_SPAWN_HEADER_SIZE + len + 1, payload_spawn_request);
                if (!AXIOM_RET_IS_OK(_msg)) goto release;
                pargs++;
            }
        }

        if (envp != NULL) {
            char * const *pargs = envp;
            payload_spawn_request->flags = 0;
            payload_spawn_request->type = AXIOM_SPAWN_TYPE_ENV;
            while (*pargs != NULL) {
                len = strlcpy((char*) payload_spawn_request->data, *pargs, sizeof (payload_spawn_request->data));
                logmsg(LOG_DEBUG, "axinit_execvpe: send ENV spawn message len=%d '%s'", len, (char*) payload_spawn_request->data);
                if (len+1>=AXIOM_SPAWN_MAX_DATA_SIZE) {
                    logmsg(LOG_WARN, "axinit_execvpe: send ARG spawn message TRUNCATED! size>%d",AXIOM_SPAWN_MAX_DATA_SIZE);
                }
                _msg = axiom_send(dev, node, AXIOM_RAW_PORT_INIT, AXIOM_SPAWN_HEADER_SIZE + len + 1, payload_spawn_request);
                if (!AXIOM_RET_IS_OK(_msg)) goto release;
                pargs++;
            }
        }

        payload_spawn_request->type = AXIOM_SPAWN_TYPE_CWD;
        payload_spawn_request->flags = AXIOM_SPAWN_FLAG_EXEC;
        s = getcwd(NULL, 0);
        if (s == NULL) {
            _msg = AXIOM_RET_ERROR;
            goto release;
        }
        len = strlcpy((char*) payload_spawn_request->data, s, sizeof (payload_spawn_request->data));
        free(s);
        logmsg(LOG_DEBUG, "axinit_execvpe: send CWD spawn message '%s'", (char*) payload_spawn_request->data);
        _msg = axiom_send_raw(dev, node, AXIOM_RAW_PORT_INIT, AXIOM_TYPE_RAW_DATA, AXIOM_SPAWN_HEADER_SIZE + len + 1, payload_spawn_request);
        if (!AXIOM_RET_IS_OK(_msg)) goto release;

        start_counter++;
        if (!broadcast) break;

    } while (++node < axiom_get_num_nodes(dev));

    logmsg(LOG_DEBUG, "axinit_execvpe: end");
    return start_counter;

release:
    payload_session_req->command = AXIOM_CMD_SESSION_REQ;
    payload_session_req->reply_port = port;
    logmsg(LOG_DEBUG, "axinit_execvpe: send sessione release reqeust to node %d", node);
    axiom_send_raw(dev, node, AXIOM_RAW_PORT_INIT, AXIOM_TYPE_RAW_DATA, sizeof (*payload_session_req), payload_session_req);
    logmsg(LOG_DEBUG, "axinit_execvpe: end (on error)");
    return _msg;
}
