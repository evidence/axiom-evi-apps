/*
 * axiom_pong.h
 *
 * Version:     v0.3.1
 * Last update: 2016-04-14
 *
 * This file defines prototypes of AXIOM traceroute reply application
 *
 */
#ifndef AXIOM_TR_REPLY_H
#define AXIOM_TR_REPLY_H

/*
 * @brief This function implements the reply to the traceroute message.
 * @param dev                   The axiom device private data pointer
 * @param if_src                Source interface of traceroute message
 * @param payload               Payload of traceroute message
 * @param verbose               enable verbose output
 */
void axiom_traceroute_reply(axiom_dev_t *dev, axiom_if_id_t src,
        axiom_payload_t payload, int verbose);

#endif /*! AXIOM_TR_REPLY_H*/
