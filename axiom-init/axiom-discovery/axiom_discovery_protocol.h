#ifndef AXIOM_DSCV_PROTOCOL_H
#define AXIOM_DSCV_PROTOCOL_H
/*!
 * \file axiom_discovery_protocol.h
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the defines and prototypes of axiom discovery phase.
 */
#include "dprintf.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_discovery.h"
#include "axiom_sim_topology.h"

/*! \brief Master node ID */
#define AXIOM_MASTER_ID                 0
/*! \brief Value returned by axiom_get_if_info() for a connected interface */
#define AXIOM_IF_CONNECTED              0x01

/*
 * \brief This function implements the Master node Discovery Algorithm.
 *
 * \param dev               The axiom device private data pointer
 * \param[out] topology     Final network topology discovered
 * \param[out] total_nodes  Number of nodes discovered into the network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
int
axiom_master_node_discovery(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_node_id_t *total_nodes);

/*
 * \brief This function implements the Slave nodes Discovery Algorithm.
 *
 * \param dev               The axiom device private data pointer
 * \param[out] topology     Final network topology discovered
 * \param[out] node_id      Node id assigned during the discovery
 * \param first_interface   Interface where the discovery message is received
 * \param first_msg         Payload of discovery message received
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
int
axiom_slave_node_discovery (axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_node_id_t *node_id,
        axiom_if_id_t first_interface, axiom_discovery_payload_t *first_msg);

#endif  /* !AXIOM_DSCV_PROTOCOL_H */
