/**
 * Axiom process spawn (i.e. exec) service management.
 *
 * @file axiom_spawn.c
 * @version v0.7
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"
#include "axiom_common.h"

#include "../axiom-init.h"

extern void axiom_session_release(uint8_t ses);
extern int axiom_session_is_used(uint8_t ses);

/*
 * SPAWN manager
 */

/** Max number of concurrent axiom spawn request managed. */
#define MAX_CONCURRENT_SPAWNS 4

#define EMPTY_APPLICATION_ID AXIOM_SESSION_EMPTY

/** Spwan information. */
typedef struct {
    uint8_t session_id; /**< Session identification */
    char *exec; /**< Executable*/
    char *cwd; /**< Working directory */
    strlist_t args; /**< Executable arguments */
    strlist_t env; /**< Executable environment */
} info_t;

/** Persist spawn information. */
static info_t info[MAX_CONCURRENT_SPAWNS];

/**
 * Initilalize spwan subsystem.
 * - fill static variables
 * - install SIGCHILD signal handler
 */
void axiom_spawn_init() {
    int i;
    memset(info, 0, sizeof (info));
    // paranoia
    for (i=0;i<MAX_CONCURRENT_SPAWNS;i++) {
        info[i].session_id = EMPTY_APPLICATION_ID;
        info[i].exec = NULL;
        info[i].cwd = NULL;
        sl_init(&info[i].args);
        sl_init(&info[i].env);
    }
    //
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        EPRINTF("SPAWN - init error setting SIGCHLD to IGNORE... but continue...");
    }
}

/**
 * Spawn request management.
 * Invoked when a AXIOM_CMD_SPAWN_REQ arrive.
 *
 * @param dev the axiom device.
 * @param src The node source.
 * @param payload_size The size of the message,
 * @param _payload the payload.
 * @param verbose 1 to emit verbose messages.
 */
void axiom_spawn_req(axiom_dev_t *dev, axiom_node_id_t src, size_t payload_size, void *_payload, int verbose) {

    axiom_spawn_req_payload_t *payload = ((axiom_spawn_req_payload_t *) _payload);
    int idx, idx_free;
    int size;

    if (payload->command != AXIOM_CMD_SPAWN_REQ) {
        EPRINTF("SPAWN - receive a not AXIOM_SPAWN_REQ message");
        return;
    }
    IPRINTF(verbose, "SPAWN - REQ message received - src_node: %u session_id: %u type: %u", src, payload->session_id, payload->type);

    // paranoia check
    if (!axiom_session_is_used(payload->session_id)) {
        EPRINTF("SPAWN - bad axiom session!");
        return;
    }

    // find my session or a free one
    for (idx = 0, idx_free = -1; idx < MAX_CONCURRENT_SPAWNS; idx++) {
        if (info[idx].session_id == EMPTY_APPLICATION_ID) idx_free = idx;
        if (info[idx].session_id == payload->session_id) break;
    }

    if (idx >= MAX_CONCURRENT_SPAWNS) {
        // no "my session found"
        if (idx_free == -1) {
            // no free session found
            DPRINTF("SPAWN - too much processo to spawn!");
            return;
        }
        // allocate the free session)
        idx = idx_free;
        info[idx].session_id = payload->session_id;
        payload->flags|=AXIOM_SPAWN_FLAG_RESET; // auto reset!!!
    }

    // if FLAG_RESET...
    // set the spawn session to default values
    if (payload->flags & AXIOM_SPAWN_FLAG_RESET) {
        if (info[idx].exec != NULL) free(info[idx].exec);
        info[idx].exec = NULL;
        if (info[idx].cwd != NULL) free(info[idx].cwd);
        info[idx].cwd = NULL;
        sl_free(&info[idx].args);
        sl_free(&info[idx].env);
    }

    // manage message types...
    switch (payload->type) {
            // if the exec messgae arrive...
        case AXIOM_SPAWN_TYPE_EXE:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE - 1] = '\0'; // paranoia
            size = strlen((char*) payload->data) + 1;
            if (info[idx].exec != NULL) free(info[idx].exec);
            info[idx].exec = malloc(size);
            if (info[idx].exec == NULL) {
                EPRINTF("SPAWN - malloc() fail!");
            } else {
                strlcpy(info[idx].exec, (char*) payload->data, size);
            }
            break;
            // yf the working directory arrive...
        case AXIOM_SPAWN_TYPE_CWD:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE - 1] = '\0'; // paranoia
            size = strlen((char*) payload->data) + 1;
            if (info[idx].cwd != NULL) free(info[idx].cwd);
            info[idx].cwd = malloc(size);
            if (info[idx].cwd == NULL) {
                EPRINTF("SPAWN - malloc() fail!");
            } else {
                strlcpy(info[idx].cwd, (char*) payload->data, size);
            }
            break;
            // if an arguments arrive...
        case AXIOM_SPAWN_TYPE_ARG:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE - 1] = '\0'; // paranoia
            sl_append(&info[idx].args, (char*) payload->data);
            break;
            // if a environemnt variable arrive...
        case AXIOM_SPAWN_TYPE_ENV:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE-1]='\0'; // paranoia
            sl_append(&info[idx].env, (char*) payload->data);
            break;
            // unknown message type!!!!
        default:
            EPRINTF("SPAWN - unknow type %u", payload->type);
            break;
    }

    // if the EXEC flag arrive... exec the application...
    if (payload->flags & AXIOM_SPAWN_FLAG_EXEC) {
        sl_append(&info[idx].args, NULL);
        sl_append(&info[idx].env, NULL);
        //
        daemonize(info[idx].cwd, info[idx].exec, sl_get(&info[idx].args), sl_get(&info[idx].env), NULL, 1, verbose);
        // release the session info to default values...
        sl_free(&info[idx].env);
        sl_free(&info[idx].args);
        if (info[idx].exec != NULL) {
            free(info[idx].exec);
            info[idx].exec = NULL;
        }
        if (info[idx].cwd != NULL) {
            free(info[idx].cwd);
            info[idx].cwd = NULL;
        }
        //
        // NB: the axiom session is automatically release!!!!
        //
        axiom_session_release(info[idx].session_id);
        info[idx].session_id = EMPTY_APPLICATION_ID;
    }

}
