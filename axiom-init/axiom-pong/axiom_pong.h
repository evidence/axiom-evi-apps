/*
 * axiom_pong.h
 *
 * Version:     v0.3.1
 * Last update: 2016-04-14
 *
 * This file defines prototypes of AXIOM pong application
 *
 */
#ifndef AXIOM_PONG_H
#define AXIOM_PONG_H

/*
 * @brief This function implements the reply (pong) of ping message.
 * @param dev                   The axiom device private data pointer
 * @param first_src             Sender of ping message
 * @param first_payload         Payload of ping message
 * @param verbose               enable verbose output
 */
void axiom_pong(axiom_dev_t *dev, axiom_if_id_t first_src,
        axiom_payload_t first_payload, int verbose);

#endif /*! AXIOM_PONG_H*/
