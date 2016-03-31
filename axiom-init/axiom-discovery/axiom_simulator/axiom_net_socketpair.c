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

#ifndef AXIOM_SIM
#define AXIOM_SIM   1
#endif
#include "axiom_nic_packets.h"
#include "axiom_nic_routing.h"
#include "axiom_simulator.h"
#include "axiom_net.h"

typedef struct axiom_net {
    int node_if_fd[AXIOM_NUM_INTERFACES];
    int num_of_ended_rt;
} axiom_net_t;

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
    DPRINTF("Socket pair: [%d,%d]\n\r", sockets[0], sockets[1]);

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
	nodes[i].net->num_of_ended_rt = 0;
    }

    for (i = 0; i < tpl->num_nodes; i++) {
        for (j = 0; j < tpl->num_interfaces; j++) {
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
 * @param flag flag the small message
 * @param payload Data to send
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_send_small_neighbour(axiom_dev_t *dev, axiom_if_id_t src_interface,
        axiom_port_t port, axiom_flag_t flag, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    ssize_t write_ret;

    /* Header */
    message.header.tx.port_flag.field.port = port;
    message.header.tx.port_flag.field.flag = flag;
    message.header.tx.dst = src_interface;

    /* Payload */
    message.payload = *payload;


    write_ret = write( ((axiom_sim_node_args_t*)dev)->net->node_if_fd[src_interface],
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
 * @param flag flag the small message
 * @param payload Data to receive
 * return Returns ...
 */
axiom_msg_id_t
axiom_net_send_small(axiom_dev_t *dev, axiom_if_id_t dest_node_id,
                              axiom_port_t port, axiom_flag_t flag,
                              axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    ssize_t write_ret;

    if (port == AXIOM_SMALL_PORT_DISCOVERY)
    {
        uint8_t if_index;

        /* Header message */
        message.header.tx.port_flag.field.port = port;
        message.header.tx.port_flag.field.flag = flag;
        message.header.tx.dst = dest_node_id;

        /* Payload */
        message.payload= *payload;

        for (if_index = 0; if_index < AXIOM_NUM_INTERFACES; if_index ++)
        {
           if (((axiom_sim_node_args_t*)dev)->local_routing[dest_node_id][if_index] == 1)
           {
                write_ret = write( ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index], &message, sizeof(message));

                if ((write_ret == -1) || (write_ret == 0))
                {
                    return AXIOM_RET_ERROR;
                }
                else
                {
                    DPRINTF("routing: send on socket = %d to node %d: (%d,%d)",
                       ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index], message.header.tx.dst,
                       (uint8_t)((message.payload & 0x00FF0000) >> 16), (uint8_t)((message.payload & 0x0000FF00) >> 8));
                }
                break;
           }
        }
    }

    return AXIOM_RET_OK;
}


/*
 * @brief This function receives small neighbour data.
 * @param dev The axiom device private data pointer
 * @param src_interface The interface on which the message is recevied
 * @param port port of the small message
 * @param flag flags of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
axiom_msg_id_t
axiom_net_recv_small_neighbour(axiom_dev_t *dev, axiom_node_id_t *src_interface,
        axiom_port_t *port, axiom_flag_t *flag, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    struct pollfd fds[AXIOM_NUM_INTERFACES];
    int i, read_counter, interface_counter;
    ssize_t read_bytes;

    NDPRINTF("Wait to receive from the following interfaces:");

    interface_counter = 0;
    for (i = 0; i < AXIOM_NUM_INTERFACES; i++)
    {
        /* get the socket descriptor (or zero value) of each node interface */
        fds[i].fd =  ((axiom_sim_node_args_t*)dev)->net->node_if_fd[i];

        if (fds[i].fd != 0)
        {
            /* Really exist a connection (a socket descriptor) on the 'if_number' interface of the
             *  node managed by the running thread.
             *  */
            fds[i].events = POLLIN;
            interface_counter++;

            NDPRINTF("fds[%d].fd = %d", i,  ((axiom_sim_node_args_t*)dev)->net->node_if_fd[i]);
        }
    }

    read_counter = poll( fds, interface_counter, -1);

    if (read_counter == -1)
    {
        return AXIOM_RET_ERROR;
    }
    else if (read_counter == 0)
    {
        /* timeout; no event detected */
    }
    else
    {
        for (i = 0; i < interface_counter; i++)
        {
            /* If we detect the event, zero it out so we can reuse the structure */
            if (fds[i].revents & POLLIN)
            {
                /* zero the event out so we can reuse the structure */
                fds[i].revents = 0;

                /* read the received message */
                read_bytes = read(fds[i].fd, &message, sizeof(message));
                if (read_bytes == sizeof(message))
                {
                    NDPRINTF("Receive from socket number = %d",
                            ((axiom_sim_node_args_t*)dev)->net->node_if_fd[i]);

                    /* Header */
                    *src_interface = i; /* index of the interface from which I have received */
                    *port = message.header.rx.port_flag.field.port;
                    *flag = message.header.rx.port_flag.field.flag;

                    *payload = message.payload;

                    break; /* socket found, exit from cycle */
                }
                else
                {
                    return AXIOM_RET_ERROR;
                }
            }
        }
    }

    return AXIOM_RET_OK;
}

/*
 * @brief This function receives small data.
 * @param dev The axiom devive private data pointer
 * @param src_node_id The node which has sent the received message
 * @param port port of the small message
 * @param flag flags of the small message
 * @param payload data received
 * @return Returns -1 on error!
 */
axiom_msg_id_t
axiom_net_recv_small(axiom_dev_t *dev, axiom_node_id_t *src_node_id,
        axiom_port_t *port, axiom_flag_t *flag, axiom_payload_t *payload)
{
    axiom_small_msg_t message;
    axiom_routing_payload_t rt_message;
    ssize_t read_bytes;

    axiom_node_id_t my_id, node_index, node_id;
    axiom_if_id_t if_index, if_id;
    int do_flag, num_nodes_after_me;


    my_id = axiom_get_node_id(dev);

    /* cycle to found the number of nodes with id > my_id that connected to me
       into my local routing table; they represent the nodes to which I have
       to send packets that Master has to send them through me */
    num_nodes_after_me = 1;
    for (node_index = my_id + 1; node_index < AXIOM_NUM_NODES; node_index++)
    {
        for (if_index = 0; if_index < AXIOM_NUM_INTERFACES; if_index ++)
        {
            if (((axiom_sim_node_args_t*)dev)->local_routing[node_index][if_index] == 1)
            {
                num_nodes_after_me++;
                if_index = AXIOM_NUM_INTERFACES;
            }
        }
    }

    /* cycle to found the smallest node id connected to me into my local routing table;
       this represents the node who sent me: "Say your ID" into discovery phase */
    do_flag = 1;
    for (node_index = 0; (node_index < my_id) && (do_flag == 1); node_index++)
    {
        for (if_index = 0; (if_index < AXIOM_NUM_INTERFACES) && (do_flag == 1); if_index ++)
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

    /* while Master has not ended to send my table and the table of the nodes
       that are connected to me  */
    while (((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt < num_nodes_after_me)
    {
        read_bytes = read(((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index], &message, sizeof(message));

        if (read_bytes != sizeof(message))
        {
            return AXIOM_RET_ERROR;
        }
        if (message.header.rx.port_flag.field.port == AXIOM_SMALL_PORT_ROUTING)
        {
            memcpy(&rt_message, &message.payload, sizeof(rt_message));

            *port = message.header.rx.port_flag.field.port;
            node_id = rt_message.node_id;
            if_id = rt_message.if_id;
            DPRINTF("routing: received on socket = %d for node %d: (%d,%d) [I'm node %d]",
                       ((axiom_sim_node_args_t*)dev)->net->node_if_fd[if_index],
                       message.header.tx.dst, node_id, if_id, my_id);

            if (rt_message.command ==  AXIOM_RT_CMD_INFO)
            {
                if (message.header.tx.dst == my_id)
                {
                    /* it is my routing table, return the info to the caller */
                    *src_node_id = message.header.tx.dst;
                    *payload = message.payload;
                    return AXIOM_RET_OK;
                }
                else
                {
                    /* I have to forward this info to recipient node*/
                    axiom_net_send_small(dev, message.header.tx.dst,
                                                  message.header.rx.port_flag.field.port,
                                                  message.header.rx.port_flag.field.flag,
                                                  &(message.payload));
                }
            }
            else if (rt_message.command == AXIOM_RT_CMD_END_INFO)
            {
                /* increment the number of ended received table */
                ((axiom_sim_node_args_t*)dev)->net->num_of_ended_rt++;

                if (message.header.tx.dst != my_id)
                {
                    axiom_net_send_small(dev, message.header.tx.dst,
                                                  message.header.rx.port_flag.field.port,
                                                  message.header.rx.port_flag.field.flag,
                                                  &(message.payload));
                }
            }
        }
    }

    /* while end: Master has ended to send my table and the table of the nodes
       that are connected to me are  */
    /*data |= (AXIOM_RT_TYPE_END_INFO << 24); */

    rt_message.command = AXIOM_RT_CMD_END_INFO;
    *payload = message.payload;

    return AXIOM_RET_OK;

}
