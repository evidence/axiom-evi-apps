#ifndef AXIOM_NET_h
#define AXIOM_NET_h

typedef struct axiom_net axiom_net_t;

/*
 * @brief This function sets up the nodes resources for discovery
 *        algorithm execution
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns -1 in case of error
 *                 0 otherwise
 */
int axiom_net_setup(axiom_sim_node_args_t *nodes, axiom_sim_topology_t *tpl);

/*
 * @brief This function frees the memory allocated during the start
 *        up nodes phase
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns none
 */
void axiom_net_free(axiom_sim_node_args_t *nodes, axiom_sim_topology_t *tpl);

/*
 * @brief This function returns the connected status of the input
 *        node interface
 * @param dev pointer to the node information
 * @param if_number node interface identifier
 * return Returns 1 interface connected
 *                0 otherwise
 */
int axiom_net_connect_status(axiom_dev_t *dev, axiom_if_id_t if_number);

/*
 * @brief This function sends a small message to a neighbour on a specific
 *        interface.
 * @param dev The axiom devive private data pointer
 * @param src_interface Sender interface identification
 * @param type type of the raw message
 * @param data Data to send
 * return Returns ...
 */
axiom_msg_id_t axiom_net_send_small_neighbour(axiom_dev_t *dev,
        axiom_if_id_t src_interface, axiom_port_t port, axiom_flag_t flag,
        axiom_payload_t *payload);

/*
 * @brief This function sends a small message to a node.
 * @param dev The axiom device private data pointer
 * @param dest_node_id Receiver node identification
 * @param port port the small message
 * @param flag flag the small message
 * @param payload Data to receive
 * return Returns ...
 */
axiom_msg_id_t axiom_net_send_small(axiom_dev_t *dev, axiom_if_id_t dest_node_id,
                              axiom_port_t port, axiom_flag_t flag,
                              axiom_payload_t *payload);

/*
 * @brief This function receives small neighbour data.
 * @param dev The axiom device private data pointer
 * @param src_interface The interface on which the message is recevied
 * @param port port of the small message
 * @param flag flags of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
axiom_msg_id_t axiom_net_recv_small_neighbour(axiom_dev_t *dev,
        axiom_node_id_t *src_interface, axiom_port_t *port, axiom_flag_t *flag,
        axiom_payload_t *payload);

/*
 * @brief This function receives small data.
 * @param dev The axiom devive private data pointer
 * @param src_node_id The node which has sent the received message
 * @param port port of the small message
 * @param flag flags of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
axiom_msg_id_t axiom_net_recv_small(axiom_dev_t *dev,
        axiom_node_id_t *src_node_id, axiom_port_t *port, axiom_flag_t *flag,
        axiom_payload_t *payload);

#endif /* !AXIOM_NET_h */
