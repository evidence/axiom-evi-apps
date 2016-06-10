/*!
 * \file axiom_barrier.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the axiom-ping/pong messages.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "../axiom-init.h"

#define MAX_CONCURRENT_BARRIERS 16

// TODO: using dynamic number of nodes!!!
#define NUM_NODES 2

#define EMPTY_APPLICATION_ID 0

struct {
    uint8_t application; // application identification
    uint8_t barrier;     // barrier identification
    uint8_t reply_port;  // port for the reply message
    //
    uint8_t counter;     // request to receive (to send a reply)
    uint8_t num_nodes;   // number of nodes
} barrier[MAX_CONCURRENT_BARRIERS];

void axiom_barrier_init() {
    memset(barrier, 0, sizeof (barrier));
}

void axiom_barrier_req(axiom_dev_t *dev, axiom_node_id_t src, axiom_payload_size_t payload_size, axiom_init_payload_t *_payload, int verbose) {

    /*
     * HP:
     * -there are only NUM_NODES nodes
     * -the nodes are sequential starting from zero
     * -source sender is not considered (the number of request are counted)
     */
    
    axiom_barrier_req_payload_t *payload = ((axiom_barrier_req_payload_t *) _payload);

    if (payload->command != AXIOM_CMD_BARRIER_REQ) {
        EPRINTF("receive a not AXIOM_BARRIER_REQ message");
        return;
    }
    IPRINTF(verbose, "BARRIER REQ message received - src_node: %u application: %u barrier: %u", src, payload->application, payload->barrier);

    if (payload->flags&AXIOM_BARRIER_FLAG_RESET) {
        int idx;
        DPRINTF("axiom barrier - reset for application id %d",payload->application);
        for (idx = 0 ; idx < MAX_CONCURRENT_BARRIERS; idx++) {
            if (barrier[idx].application == payload->application) barrier[idx].application = EMPTY_APPLICATION_ID;
        }
        return;        
    }
    
    
    int idx, idx_free;
    for (idx = 0, idx_free = -1; idx < MAX_CONCURRENT_BARRIERS; idx++) {
        if (barrier[idx].application == EMPTY_APPLICATION_ID) idx_free = idx;
        if (barrier[idx].application == payload->application && barrier[idx].barrier == payload->barrier) break;
    }
    
    if (idx >= MAX_CONCURRENT_BARRIERS) {
        if (idx_free == -1) {
            IPRINTF(verbose, "BARRIER - too much barriers... reply with error!");
            axiom_barrier_reply_payload_t *payload2 = ((axiom_barrier_reply_payload_t *) _payload);
            payload2->command = AXIOM_CMD_BARRIER_REPLY;
            payload2->result=255;
            axiom_err_t ret = axiom_send_raw(dev, src, payload->reply_port, AXIOM_TYPE_RAW_DATA, sizeof(*payload2), payload2);
            if (ret == AXIOM_RET_ERROR) {
                EPRINTF("send small error");
                return;
            }
            return;
        }
        idx = idx_free;
        barrier[idx].application = payload->application;
        barrier[idx].barrier = payload->barrier;
        barrier[idx].reply_port=payload->reply_port;
        barrier[idx].counter = NUM_NODES; // WARNIG: TODO: nuber of active nodes!!!
        barrier[idx].num_nodes = NUM_NODES;
    }

    barrier[idx].counter--;
    if (barrier[idx].counter==0) {                
        barrier[idx].application=0;        
        
        axiom_barrier_reply_payload_t *payload2 = ((axiom_barrier_reply_payload_t *) _payload);
        payload2->command = AXIOM_CMD_BARRIER_REPLY;
        payload2->result=0;
        DPRINTF("axiom barrier - payload size: %ld",sizeof(*payload2));
        
        int dst;
        for (dst=0;dst<barrier[idx].num_nodes;dst++) {
            axiom_err_t ret = axiom_send_raw(dev, dst, barrier[idx].reply_port, AXIOM_TYPE_RAW_DATA, sizeof(*payload2), payload2);
            if (ret == AXIOM_RET_ERROR) {
                EPRINTF("send small error to node %u",dst);
            }            
        }                
    }
}
