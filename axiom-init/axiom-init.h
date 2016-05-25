#ifndef AXIOM_INIT_h
#define AXIOM_INIT_h
/*!
 * \file axiom-init.h
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the control axiom messages.
 */
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
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_if_id_t routing_table[AXIOM_NODES_MAX], int verbose);

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
axiom_discovery_slave(axiom_dev_t *dev,
        axiom_node_id_t first_src, axiom_init_payload_t *first_payload,
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_if_id_t routing_table[AXIOM_NODES_MAX], int verbose);

/*!
 * \brief This function implements the reply (pong) of ping message.
 *
 * \param dev                   The axiom device private data pointer
 * \param first_src             Sender of ping message
 * \param first_payload         Payload of ping message
 * \param verbose               Enable verbose output
 */
void
axiom_pong(axiom_dev_t *dev, axiom_if_id_t first_src,
        axiom_init_payload_t *first_payload, int verbose);

/*!
 * \brief This function implements the reply to the netperf message.
 *
 * \param dev                   The axiom device private data pointer
 * \param src                   Source node of netperf message
 * \param payload_size          Size of payload of netperf message
 * \param payload               Payload of netperf message
 * \param verbose               enable verbose output
 */
void
axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src,
        axiom_payload_size_t payload_size,
        axiom_init_payload_t *payload, int verbose);

/*!
 * \brief This function implements the reply to the traceroute message.
 *
 * \param dev                   The axiom device private data pointer
 * \param if_src                Source interface of traceroute message
 * \param payload               Payload of traceroute message
 * \param verbose               enable verbose output
 */
void
axiom_traceroute_reply(axiom_dev_t *dev, axiom_if_id_t src,
        axiom_init_payload_t *payload, int verbose);

#endif /*! AXIOM_INIT_h*/
