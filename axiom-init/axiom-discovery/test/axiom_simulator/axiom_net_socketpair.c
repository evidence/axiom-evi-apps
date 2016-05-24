#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_nic_packets.h"
#include "axiom_nic_routing.h"
#include "axiom_simulator.h"
#include "axiom_net.h"

typedef struct axiom_net {
    int node_if_fd[AXIOM_INTERFACES_MAX];
    int num_of_ended_rt;
    /* If send_recv_small_sim == 0 , axiom_net_send_small() and
     * axiom_net_send_small() must manage the flow of the message from MASTER to
     * the Slave (AXIOM_RT_CMD_INFO and AXIOM_RT_CMD_END_INFO messages),
     * otherwise it must manage the flow of the message from Slaves to MASTER
     * (AXIOM_RT_CMD_RT_REPLY messages) */
    int send_recv_small_sim;
    int num_recv_reply[AXIOM_INTERFACES_MAX];
} axiom_net_t;

/*
 * @brief This function manages a small message send from Master
 *        to a slave (using local routing table of the node).
 * @param dev The axiom device private data pointer
 * @param dest_node_id Receiver node identification
 * @param port port the small message
 * @param type type the small message
 * @param payload Data to receive
 * return Returns ...
 */
static axiom_msg_id_t
send_from_master_to_slave(axiom_dev_t *dev, axiom_if_id_t dest_node_id,
        axiom_port_t port, axiom_type_t type, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    ssize_t write_ret;
    int last_node = 1;
    uint8_t if_i;
    axiom_routing_payload_t rt_payload;

    if (port != AXIOM_SMALL_PORT_INIT)
    {
        return AXIOM_RET_ERROR;
    }
    uint8_t if_index;

    /* Header message */
    message.header.tx.port_type.field.port = port;
    message.header.tx.port_type.field.type = type;
    message.header.tx.dst = dest_node_id;

    /* Payload */
    message.payload= *payload;

    for (if_index = 0; if_index < AXIOM_INTERFACES_MAX; if_index ++)
    {
        if (((axiom_sim_node_args_t*)dev)->local_routing[dest_node_id][if_index] == 1)
        {
            write_ret = write(((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
                    &message, sizeof(message));

            if ((write_ret == -1) || (write_ret == 0))
            {
                return AXIOM_RET_ERROR;
            }
            DPRINTF("routing: send on socket = %d to node %d: (%d,%d)",
                    ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
                    message.header.tx.dst, (uint8_t)((message.payload &
                            0x00FF0000) >> 16), (uint8_t)((message.payload &
                                0x0000FF00) >> 8));
            break;
        }
    }


    /* MASTER changing of axiom_net_send_small() simulation mode */
    if (((axiom_sim_node_args_t*)dev)->node_id != AXIOM_MASTER_ID)
    {
        return AXIOM_RET_OK;
    }
    rt_payload = *(axiom_routing_payload_t*)payload;
    if (rt_payload.command == AXIOM_RT_CMD_END_INFO)
    {
        /* check if the node is the last into the topology */
        if (dest_node_id != (AXIOM_NODES_MAX -1))
        {
            for (if_i = 0; if_i < AXIOM_INTERFACES_MAX; if_i ++)
            {
                if (((axiom_sim_node_args_t*)dev)->local_routing[dest_node_id+1][if_i] == 1)
                {
                    /* Exist 'dest_node_id+1', so dest_node_id it is not the last node*/
                    last_node = 0;
                }
            }
        }

        if ((last_node == 1) || (dest_node_id == (AXIOM_NODES_MAX -1)))
        {
            /* the next axiom_net_recv_small() simulation has to be different
               (AXIO_RT_CMD_RT_REPLY) */
            ((axiom_sim_node_args_t*)dev)->net->send_recv_small_sim = 1;
        }
    }
    return AXIOM_RET_OK;
}

/*
 * @brief This function manages a small message receiption
 *          during routing table delivery.
 * @param dev The axiom devive private data pointer
 * @param src_node_id The node which has sent the received message
 * @param port port of the small message
 * @param type type of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
static axiom_msg_id_t
recv_from_master_to_slave(axiom_dev_t *dev, axiom_node_id_t *src_node_id,
        axiom_port_t *port, axiom_type_t *type, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    axiom_routing_payload_t rt_message;
    ssize_t read_bytes;

    axiom_node_id_t node_id, node_index;
    axiom_if_id_t if_index;
    int do_flag, num_nodes_after_me;

    node_id = axiom_get_node_id(dev);

    /* cycle to found the number of nodes with id > node_id that connected to me
       into local local routing table; they represent the nodes to which I have
       to send packets that Master has to send them through me */
    num_nodes_after_me = 1;
    for (node_index = node_id + 1; node_index < AXIOM_NODES_MAX; node_index++)
    {
        for (if_index = 0; if_index < AXIOM_INTERFACES_MAX; if_index ++)
        {
            if (((axiom_sim_node_args_t*)dev)->local_routing[node_index][if_index] == 1)
            {
                num_nodes_after_me++;
                if_index = AXIOM_INTERFACES_MAX;
            }
        }
    }

    /* cycle to found the smallest node id connected to me into local local routing table;
       this represents the node who sent me: "Say your ID" into discovery phase */
    do_flag = 1;
    for (node_index = 0; (node_index < node_id) && (do_flag == 1); node_index++)
    {
        for (if_index = 0; (if_index < AXIOM_INTERFACES_MAX) && (do_flag == 1);
                if_index ++)
        {
            if (((axiom_sim_node_args_t*)dev)->local_routing[node_index][if_index] == 1)
            {
                do_flag = 0;
            }
        }
    }
    if (if_index != 0)
    {
        if_index--;
    }

    /* while Master has not ended to send local table and the table of the nodes
       that are connected to me  */
    while (((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt < num_nodes_after_me)
    {
        read_bytes = read(((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
                &message, sizeof(message));

        if (read_bytes != sizeof(message))
        {
            return AXIOM_RET_ERROR;
        }
        if (message.header.rx.port_type.field.port != AXIOM_SMALL_PORT_INIT)
        {
            continue;
        }
        memcpy(&rt_message, &message.payload, sizeof(rt_message));

        *port = message.header.rx.port_type.field.port;
        DPRINTF("routing: received on socket = %d for node %d: (%d,%d) "
                "[I'm node %d]",
                ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
                message.header.tx.dst, rt_message.node_id, rt_message.if_mask, node_id);

        if (rt_message.command ==  AXIOM_RT_CMD_INFO)
        {
            if (message.header.tx.dst == node_id)
            {
                /* it is local routing table, return the info to the caller */
                *src_node_id = message.header.tx.dst;
                *payload = message.payload;
                return AXIOM_RET_OK;
            }
            /* I have to forward this info to recipient node*/
            axiom_net_send_small(dev, message.header.tx.dst,
                    message.header.rx.port_type.field.port,
                    message.header.rx.port_type.field.type,
                    &(message.payload));
        }
        else if (rt_message.command == AXIOM_RT_CMD_END_INFO)
        {
            /* increment the number of ended received table */
            ((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt++;

            if (message.header.tx.dst != node_id)
            {
                axiom_net_send_small(dev, message.header.tx.dst,
                        message.header.rx.port_type.field.port,
                        message.header.rx.port_type.field.type,
                        &(message.payload));
            }
        }
    }

    /* while end: Master has ended to send local table and the table of the nodes
       that are connected to me are  */
    rt_message.command = AXIOM_RT_CMD_END_INFO;
    *payload = message.payload;

    /* Slave changing of axiom_net_send_small() simulation mode */
    ((axiom_sim_node_args_t*)dev)->net->send_recv_small_sim = 1;
    /* reset of num_of_ended_rt */
    ((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt = 0;

    return AXIOM_RET_OK;
}

/*
 * @brief This function manages a small message send from Master
 *        to a slave (using local routing table of the node).
 * @param dev The axiom device private data pointer
 * @param dest_node_id Receiver node identification
 * @param port port the small message
 * @param type type the small message
 * @param payload Data to receive
 * return Returns ...
 */
static axiom_msg_id_t
send_from_slave_to_master(axiom_dev_t *dev, axiom_if_id_t dest_node_id,
        axiom_port_t port, axiom_type_t type, axiom_payload_t *payload)
{
    axiom_node_id_t node_id, node_index;
    int do_flag, num_nodes_after_me, i;
    ssize_t read_bytes;
    axiom_small_msg_t message;
    ssize_t write_ret;
    uint8_t if_index;
    uint8_t recv_if[AXIOM_INTERFACES_MAX];

    if (port != AXIOM_SMALL_PORT_INIT)
    {
        return AXIOM_RET_ERROR;
    }

    memset (recv_if, 0, sizeof(recv_if));


    /* Header message */
    message.header.tx.port_type.field.port = port;
    message.header.tx.port_type.field.type = type;
    message.header.tx.dst = dest_node_id;

    /* Payload */
    message.payload= *payload;

    node_id = axiom_get_node_id(dev);

    /* cycle to found the number of nodes with id > node_id that connected to me
     * into local local routing table; they represent the nodes from which I have
     * to receive packets reply that them send to Master through me */
    num_nodes_after_me = 0;
    for (node_index = node_id + 1; node_index < AXIOM_NODES_MAX; node_index++)
    {
        for (if_index = 0; if_index < AXIOM_INTERFACES_MAX; if_index ++)
        {
            if (((axiom_sim_node_args_t*)dev)->local_routing[node_index][if_index] == 1)
            {
                recv_if[if_index]++;
                num_nodes_after_me++;
                if_index = AXIOM_INTERFACES_MAX;
            }
        }
    }

    /* cycle to found the smallest node id connected to me into local local routing
     * table; to whic I have to forward the reply messages that nodes with id
     * greater than mine want to send to Master */
    do_flag = 1;
    for (node_index = 0; (node_index < node_id) && (do_flag == 1); node_index++)
    {
        for (if_index = 0; (if_index < AXIOM_INTERFACES_MAX) && (do_flag == 1);
                if_index ++)
        {
            if (((axiom_sim_node_args_t*)dev)->local_routing[node_index][if_index] == 1)
            {
                do_flag = 0;
            }
        }
    }
    if (if_index == 0)
    {
        return AXIOM_RET_ERROR;
    }

    if_index--;

    /* send the message */
    write_ret = write( ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
            &message, sizeof(message));
    if ((write_ret == -1) || (write_ret == 0))
    {
        return AXIOM_RET_ERROR;
    }

    DPRINTF("reply: send on socket = %d to node %d: (%d,%d)",
            ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
            message.header.tx.dst, (uint8_t)((message.payload & 0x00FF0000) >>
                16), (uint8_t)((message.payload & 0x0000FF00) >> 8));

    i = 0;

    while (((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt < num_nodes_after_me)
    {
        if (((axiom_sim_node_args_t*)dev)->net->num_recv_reply[i] != recv_if[i])
        {
            read_bytes = read(((axiom_sim_node_args_t*)dev)->net->node_if_fd[i],
                    &message, sizeof(message));

            if (read_bytes != sizeof(message))
            {
                return AXIOM_RET_ERROR;
            }

            ((axiom_sim_node_args_t*)dev)->net->num_recv_reply[i]++;
            ((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt++;

            /* I have to forward this back to the Master */
            axiom_net_send_small(dev, message.header.tx.dst,
                    message.header.rx.port_type.field.port,
                    message.header.rx.port_type.field.type,
                    &(message.payload));
        }
        else
        {
            i++;
        }
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function manages a small message receiption from slave
 *        to master (using local routing table of the node).
 * @param dev The axiom devive private data pointer
 * @param src_node_id The node which has sent the received message
 * @param port port of the small message
 * @param type type of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
static axiom_msg_id_t recv_from_slave_to_master (axiom_dev_t *dev,
        axiom_node_id_t *src_node_id, axiom_port_t *port, axiom_type_t *type,
        axiom_payload_t *payload)
{
    axiom_node_id_t node_id, node_index;
    uint8_t if_index;
    ssize_t read_bytes;
    uint8_t recv_if = AXIOM_INTERFACES_MAX;
    axiom_small_msg_t message;

    node_id = axiom_get_node_id(dev);

    /* cycle to found the node with id > node_id that connected to me
       into local local routing table; it represents the node from which I have
       to receive packets reply that each node send to me  */
    for (node_index = node_id + 1; node_index < AXIOM_NODES_MAX; node_index++)
    {
        for (if_index = 0; if_index < AXIOM_INTERFACES_MAX; if_index++)
        {
            if (((axiom_sim_node_args_t*)dev)->local_routing[node_index][if_index] == 1)
            {
                if (recv_if == AXIOM_INTERFACES_MAX)
                {
                    /* memorize if from wich I have to receive */
                    recv_if = if_index;
                }
                break;
            }
        }
    }

    read_bytes = read(((axiom_sim_node_args_t*)dev)->net->node_if_fd[recv_if],
            &message, sizeof(message));

    if (read_bytes != sizeof(message))
    {
        return AXIOM_RET_ERROR;
    }

    *port = message.header.rx.port_type.field.port;
    *src_node_id = message.header.tx.dst;
    *payload = message.payload;

    return AXIOM_RET_OK;
}

/*
 * @brief This function allocates a socket for each node interface
 * @param nodeA AXIOM node
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns -1 in case of error
 *                 0 otherwise
 */
static int
allocate_link(uint8_t nodeA, uint8_t ifA, axiom_sim_node_args_t *nodes,
        axiom_sim_topology_t *tpl)
{
    uint8_t nodeB, ifB = AXIOM_NULL_NODE;
    int i, sockets[2];

    nodeB = tpl->topology[nodeA][ifA];

    /* The ifA of nodeA is not connected to any node */
    if (nodeB == AXIOM_NULL_NODE) {
        NDPRINTF("interface not conntected - nodeA=%d ifA=%d nodeB=%d ifB=%d",
                nodeA, ifA, nodeB, ifB);
        return 0;
    }

    /* Link already allocated */
    if (nodes[nodeA].net->node_if_fd[ifA] != 0) {
        NDPRINTF("link already allocated - nodeA=%d ifA=%d nodeB=%d ifB=%d",
                nodeA, ifA, nodeB, ifB);
        return 0;
    }

    /* Find the ifB connected to ifA */
    for (i = 0; i < tpl->num_interfaces; i++) {
        if (tpl->topology[nodeB][i] == nodeA) {
            ifB = i;
            break;
        }
    }

    if (ifB == AXIOM_NULL_INTERFACE) {
        NEPRINTF("link not found - nodeA=%d ifA=%d nodeB=%d ifB=%d",
                nodeA, ifA, nodeB, ifB);
        return -1;
    }

    /* Create a socket pair (link) between the interfaces */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        perror("opening stream socket pair");
        return -1;
    }

    nodes[nodeA].net->node_if_fd[ifA] = sockets[0];
    nodes[nodeB].net->node_if_fd[ifB] = sockets[1];
    DPRINTF("Socket pair: [%d,%d]\n", sockets[0], sockets[1]);

    return 0;
}

/*
 * @brief This function sets up the nodes resources for discovery
 *        algorithm execution
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns -1 in case of error
 *                 0 otherwise
 */
int
axiom_net_setup(axiom_sim_node_args_t *nodes, axiom_sim_topology_t *tpl)
{
    int i, j, err;

    for (i = 0; i < tpl->num_nodes; i++) {
        nodes[i].net = malloc(sizeof (axiom_net_t));
        if (!nodes[i].net) {
            axiom_net_free(nodes, tpl);
            return -ENOMEM;
        }
        memset(nodes[i].net->node_if_fd, 0, sizeof(int) * AXIOM_INTERFACES_MAX);
        nodes[i].net->num_of_ended_rt = 0;
        nodes[i].net->send_recv_small_sim = 0;
    }

    for (i = 0; i < tpl->num_nodes; i++) {
        for (j = 0; j < tpl->num_interfaces; j++) {
            nodes[i].net->num_recv_reply[j] = 0;
            err = allocate_link(i, j, nodes, tpl);
            if (err) {
                return err;
            }
        }
    }

    return 0;
}

/*
 * @brief This function frees the memory allocated during the start
 *        up nodes phase
 * @param nodes AXIOM nodes information
 * @param tpl simulated initial topology
 * return Returns none
 */
void
axiom_net_free(axiom_sim_node_args_t *nodes, axiom_sim_topology_t *tpl)
{
    int i;

    for (i = 0; i < tpl->num_nodes; i++) {
        if (nodes[i].net) {
            free(nodes[i].net);
            nodes[i].net = NULL;
        }
    }
}

/*
 * @brief This function returns the connected status of the input
 *        node interface
 * @param dev pointer to the node information
 * @param if_number node interface identifier
 * return Returns 1 interface connected
 *                0 otherwise
 */
int
axiom_net_connect_status(axiom_dev_t *dev, axiom_if_id_t if_number)
{
    return (((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_number] != 0);
}

/*
 * @brief This function sends a small message to a neighbour on a specific
 *        interface.
 * @param dev The axiom device private data pointer
 * @param src_interface Sender interface identification
 * @param port port the small message
 * @param type type the small message
 * @param payload Data to send
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_send_small_neighbour(axiom_dev_t *dev, axiom_if_id_t src_interface,
        axiom_port_t port, axiom_type_t type, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    ssize_t write_ret;

    /* Header */
    message.header.tx.port_type.field.port = port;
    message.header.tx.port_type.field.type = type;
    message.header.tx.dst = src_interface;

    /* Payload */
    message.payload = *payload;


    write_ret = write(((axiom_sim_node_args_t*)dev)->net->node_if_fd[src_interface],
            &message, sizeof(message));

    if ((write_ret == -1) || (write_ret == 0))
    {
        return AXIOM_RET_ERROR;
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function sends a small message to a node.
 * @param dev The axiom device private data pointer
 * @param dest_node_id Receiver node identification
 * @param port port the small message
 * @param type type the small message
 * @param payload Data to receive
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_send_small(axiom_dev_t *dev, axiom_if_id_t dest_node_id,
        axiom_port_t port, axiom_type_t type, axiom_payload_t *payload)
{
    axiom_msg_id_t ret;

    if (((axiom_sim_node_args_t*)dev)->net->send_recv_small_sim == 0)
    {
        /* management of messages sent from MASTER to all nodes with each node
           routing table*/
        ret = send_from_master_to_slave(dev, dest_node_id, port, type, payload);
    }
    else
    {
        /* management of messages reply from Slave to Master to confirm
         * the receiption of its the routing table */
        ret = send_from_slave_to_master(dev, dest_node_id, port, type, payload);
    }

    return ret;
}


/*
 * @brief This function receives small neighbour data.
 * @param dev The axiom device private data pointer
 * @param src_interface The interface on which the message is recevied
 * @param port port of the small message
 * @param type type of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
axiom_msg_id_t
axiom_net_recv_small_neighbour(axiom_dev_t *dev, axiom_node_id_t *src_interface,
        axiom_port_t *port, axiom_type_t *type, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    struct pollfd fds[AXIOM_INTERFACES_MAX];
    int i, read_counter, interface_counter;
    ssize_t read_bytes;

    NDPRINTF("Wait to receive from the following interfaces:");

    interface_counter = 0;
    for (i = 0; i < AXIOM_INTERFACES_MAX; i++)
    {
        /* get the socket descriptor (or zero value) of each node interface */
        fds[i].fd =  ((axiom_sim_node_args_t*)dev)->net->node_if_fd[i];

        if (fds[i].fd != 0)
        {
            /* Really exist a connection (a socket descriptor) on the
             * 'if_number' interface of the node managed by the running thread.
             */
            fds[i].events = POLLIN;
            interface_counter++;

            NDPRINTF("fds[%d].fd = %d", i,
                    ((axiom_sim_node_args_t*)dev)->net->node_if_fd[i]);
        }
    }

    read_counter = poll(fds, interface_counter, -1);

    if (read_counter == -1)
    {
        return AXIOM_RET_ERROR;
    }
    else if (read_counter == 0)
    {
        /* timeout; no event detected */
        DPRINTF("poll timeout");
        return AXIOM_RET_OK;
    }

    for (i = 0; i < interface_counter; i++)
    {
        /* If we detect the event, zero it out so we can reuse the structure */
        if (fds[i].revents & POLLIN)
        {
            /* zero the event out so we can reuse the structure */
            fds[i].revents = 0;

            /* read the received message */
            read_bytes = read(fds[i].fd, &message, sizeof(message));
            if (read_bytes != sizeof(message))
            {
                return AXIOM_RET_ERROR;
            }
            NDPRINTF("Receive from socket number = %d",
                    ((axiom_sim_node_args_t*)dev)->net->node_if_fd[i]);

            /* index of the interface from which I have received */
            *src_interface = i;
            *port = message.header.rx.port_type.field.port;
            *type = message.header.rx.port_type.field.type;

            *payload = message.payload;

            break; /* socket found, exit from cycle */
        }
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function receives small data.
 * @param dev The axiom devive private data pointer
 * @param src_node_id The node which has sent the received message
 * @param port port of the small message
 * @param type type of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
axiom_msg_id_t
axiom_net_recv_small(axiom_dev_t *dev, axiom_node_id_t *src_node_id,
        axiom_port_t *port, axiom_type_t *type, axiom_payload_t *payload)
{
    axiom_msg_id_t ret;

    if (((axiom_sim_node_args_t*)dev)->net->send_recv_small_sim == 0)
    {
        /* management of messages sent from MASTER to all nodes with each node
           routing table*/
        ret = recv_from_master_to_slave(dev, src_node_id, port, type, payload);
    }
    else
    {
        /* management of messages reply from Slave to Master to confirm
         * the receiption of its the routing table */
        ret = recv_from_slave_to_master(dev, src_node_id, port, type, payload);
    }

    return ret;
}
