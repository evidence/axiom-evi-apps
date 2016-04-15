/*
 * axiom_netperf_reply.h
 *
 * Version:     v0.3.1
 * Last update: 2016-04-15
 *
 * This file defines prototypes of AXIOM NETPERF application
 *
 */
#ifndef AXIOM_NETPERF_REPLY_h
#define AXIOM_NETPERF_REPLY_h

/*
 * @brief This function implements the reply to the netperf message.
 * @param dev                   The axiom device private data pointer
 * @param src                Source node of netperf message
 * @param payload               Payload of netperf message
 * @param verbose               enable verbose output
 */
void axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src,
        axiom_payload_t payload, int verbose);

#endif /*! AXIOM_NETPERF_REPLY_h*/
