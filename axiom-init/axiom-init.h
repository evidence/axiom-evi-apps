/*!
 * \file axiom-init.h
 *
 * \version     v0.15
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the control axiom messages.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_INIT_h
#define AXIOM_INIT_h

#include "axiom_nic_init.h"

/*!
 * \brief Discovery algorithm and routing table delivery for the master node
 *
 * \param dev                   The axiom device private data pointer
 * \param[out] topology         Final network topology discovered
 * \param[out] routing_table    Routing table generated
 * \param verbose               Enable verbose output
 */
void
axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_if_id_t routing_table[AXIOM_NODES_NUM]);

/*!
 * \brief Discovery algorithm and routing table delivery for the slave nodes
 *
 * \param dev                   The axiom device private data pointer
 * \param first_src             Interface where the discovery message is received
 * \param first_payload         Payload of discovery message
 * \param[out] topology         Final network topology discovered
 * \param[out] routing_table    Routing table received
 * \param verbose               Enable verbose output
 */
void
axiom_discovery_slave(axiom_dev_t *dev, axiom_node_id_t first_src,
        void *first_payload, axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_if_id_t routing_table[AXIOM_NODES_NUM]);

/*!
 * \brief This function implements the reply (pong) of ping message.
 *
 * \param dev                   The axiom device private data pointer
 * \param first_src             Sender of ping message
 * \param first_payload         Payload of ping message
 * \param verbose               Enable verbose output
 */
void
axiom_pong(axiom_dev_t *dev, axiom_if_id_t first_src, void *first_payload,
        int verbose);

/*!
 * \brief This function implements the reply to the traceroute message.
 *
 * \param dev                   The axiom device private data pointer
 * \param if_src                Source interface of traceroute message
 * \param payload               Payload of traceroute message
 * \param verbose               Enable verbose output
 */
void
axiom_traceroute_reply(axiom_dev_t *dev, axiom_if_id_t src,
        void *payload, int verbose);

/*!
 * \brief This function initialize the internal structures used by the spawn
 *        service implementation.
 */
void axiom_spawn_init();

/*!
 * \brief This function implement the spawn request handling..
 *
 * \param dev                   The axiom device private data pointer
 * \param src                   Source node of spawn request
 * \param payload_size          Size of payload of spawn request message
 * \param payload               Payload of spawn request message
 * \param verbose               Enable verbose output
 */
void axiom_spawn_req(axiom_dev_t *dev, axiom_node_id_t src,
        size_t payload_size, void *payload, int verbose);

/*!
 * \brief This function implement the session request handling..
 *
 * \param dev                   The axiom device private data pointer
 * \param src                   Source node of spawn request
 * \param payload_size          Size of payload of spawn request message
 * \param payload               Payload of spawn request message
 * \param verbose               Enable verbose output
 */
void axiom_session(axiom_dev_t *dev, axiom_node_id_t src, size_t payload_size,
        void *payload, int verbose);

/*!
 * \brief This function initialize the internal structures used by the allocator
 */
void axiom_allocator_l1_init();

/*!
 * \brief This function implements the AXIOM allocator L1.
 *
 * \param dev                   The axiom device private data pointer
 * \param src                   Source node of message
 * \param payload_size          Size of payload
 * \param payload               Payload of message
 * \param verbose               Enable verbose output
 */
void axiom_allocator_l1(axiom_dev_t *dev, axiom_node_id_t src,
        size_t payload_size, void *payload, int verbose);

#endif /*! AXIOM_INIT_h*/
