/*!
 * \file axiom_discovery_protocol.h
 *
 * \version     v1.2
 * \date        2016-05-03
 *
 * This file contains the defines and prototypes of axiom discovery phase.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_DSCV_PROTOCOL_H
#define AXIOM_DSCV_PROTOCOL_H

#include "dprintf.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_regs.h"
#include "axiom_nic_discovery.h"
#include "axiom_sim_topology.h"

/*! \brief Value returned by axiom_get_if_info() for a connected interface */
#define AXIOM_IF_CONNECTED              AXIOMREG_IFINFO_CONNECTED
#define AXIOM_IF_LOOPBACK               AXIOMREG_ROUTING_LOOPBACK_IF

/*
 * \brief This function implements the Master node Discovery Algorithm.
 *
 * \param dev               The axiom device private data pointer
 * \param[out] topology     Final network topology discovered
 * \param      master_id    ID of master node
 * \param[out] last_node    Last ID used into the network
 *
 * \return AXIOM_RET_OK on success, otherwise AXIOM_RET_ERROR
 */
int
axiom_master_node_discovery(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_node_id_t master_id, axiom_node_id_t *last_node);

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
        axiom_node_id_t topology[][AXIOM_INTERFACES_NUM],
        axiom_node_id_t *node_id,
        axiom_if_id_t first_interface, axiom_discovery_payload_t *first_msg);

#endif  /* !AXIOM_DSCV_PROTOCOL_H */
