#ifndef AXIOM_INIT_h
#define AXIOM_INIT_h

/*
 * @brief This function implements the master node code.
 * @param dev                   The axiom device private data pointer
 * @param topology              matrix memorizing the final network topology.
 * @param final_routing_table   final master node routing table
 * @param verbose               enable verbose output
 */
void
axiom_discovery_master(axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES], int verbose);

/*
 * @brief This function implements the slaves node code.
 * @param dev                   The axiom device private data pointer
 * @param first_src             interface where the first message is received
 * @param first_message         first message received
 * @param topology              matrix memorizing the intermediate nodes
 *                              topology.
 * @param final_routing_table   final node routing table
 * @param verbose               enable verbose output
 */
void
axiom_discovery_slave(axiom_dev_t *dev,
        axiom_node_id_t first_src, axiom_payload_t first_payload,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES], int verbose);
/*
 * @brief This function implements the reply (pong) of ping message.
 * @param dev                   The axiom device private data pointer
 * @param first_src             Sender of ping message
 * @param first_payload         Payload of ping message
 * @param verbose               enable verbose output
 */
void
axiom_pong(axiom_dev_t *dev, axiom_if_id_t first_src,
        axiom_payload_t first_payload, int verbose);
/*
 * @brief This function implements the reply to the netperf message.
 * @param dev                   The axiom device private data pointer
 * @param src                Source node of netperf message
 * @param payload               Payload of netperf message
 * @param verbose               enable verbose output
 */
void
axiom_netperf_reply(axiom_dev_t *dev, axiom_node_id_t src,
        axiom_payload_t payload, int verbose);
/*
 * @brief This function implements the reply to the traceroute message.
 * @param dev                   The axiom device private data pointer
 * @param if_src                Source interface of traceroute message
 * @param payload               Payload of traceroute message
 * @param verbose               enable verbose output
 */
void
axiom_traceroute_reply(axiom_dev_t *dev, axiom_if_id_t src,
        axiom_payload_t payload, int verbose);

#endif /*! AXIOM_INIT_h*/
