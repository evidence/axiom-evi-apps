/*!
 * \file axiom_routing.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom routing functions.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_discovery_protocol.h"
#include "axiom_routing.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_routing.h"


/* At the end of the discovery algorithm,  Master node (which knows the entire
 * network topology) performs a similar-Dijkstra algorithm in order to compute
 * the routing tables of all the nodes of the network.  This algorithm assumes
 * that each link between two nodes has an unitary cost (the shortes path has to
 * be computed) and it starts from Mastr neighbours e than to their neighbours,
 * etc...  In computing Y node routing table, each time an  Y neighbour node or
 * an Y neighbours connected node is found, the Master immediately add this X
 * node into the Y routing table; that is the first time the Master found an Y
 * connected node it adds the node into the routing table.  Since the nodes are
 * added into the routing table from Master to the increasing neighborhood
 * level, the shortest path from Y to each network node is computed.  This
 * algorithm computes only a single path between the nodes.
 */


/************************ Routing table computation ***************************/

/* init a node routing table */
static void
axiom_init_routing_neighbour(axiom_if_id_t rt[][AXIOM_NODES_MAX],
        axiom_if_id_t neighbour_table[][AXIOM_NODES_MAX])
{
    int i;

    for (i = 0; i < AXIOM_NODES_MAX; i++)
    {
        /* node routing table */
        rt[0][i] = AXIOM_NULL_RT_INTERFACE;

        /* node first and second level neighbours talbes */
        neighbour_table[0][i] = AXIOM_NULL_NODE;
        neighbour_table[1][i] = AXIOM_NULL_NODE;
    }

}

/*
 * This function computes the first level neighbours of
 * 'actual_node_id' node.
 * Return the number of founded neighbours
 */
static uint8_t
axiom_compute_neighbours_route(axiom_node_id_t actual_node_id,
        axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_if_id_t routing_tables[][AXIOM_NODES_MAX],
        axiom_node_id_t neighbour_table[][AXIOM_NODES_MAX])
{
    axiom_if_id_t interface_index, interface_index_to_set;
    axiom_node_id_t neighbour_id;
    uint8_t neighbours_counter;

    neighbours_counter = 0;

    for (interface_index = 0; interface_index < AXIOM_INTERFACES_MAX;
            interface_index++)
    {
        if (topology[actual_node_id][interface_index] == AXIOM_NULL_NODE)
        {
            continue;
        }
        /* first level neighbours--> memorize into routing table
           that the neighbour of actual_node_id is on interface
           'interface_index' */
        neighbour_id = topology[actual_node_id][interface_index];


        /* memorize the interface connected to neighbour_id */

        /* This commented solution doesn't manage the doble link
           from two nodes (Example: node 2 has IF0 and IF1
           directly conneted to node 5) */
        /* routing_tables[actual_node_id][neighbour_id] = interface_index; */

        interface_index_to_set = routing_tables[actual_node_id][neighbour_id];
        interface_index_to_set |= (axiom_if_id_t)(1 << interface_index);
        routing_tables[actual_node_id][neighbour_id] = interface_index_to_set;

        /* save node id of the neighbour */
        neighbour_table[0][neighbour_id] = neighbour_id;

        neighbours_counter++;
    }

    return neighbours_counter;
}


void
axiom_compute_routing_tables(axiom_node_id_t topology[][AXIOM_INTERFACES_MAX],
        axiom_if_id_t routing_tables[][AXIOM_NODES_MAX],
        axiom_node_id_t total_nodes)
{
    /* ID of the node of of which the master is computing the routing table*/
    axiom_node_id_t actual_node_id;
    /* temp_rt[0] contains info about the n-level 'actual_node_id' neighbours*/
    /* temp_rt[1] contains info about the (n+1)-level 'actual_node_id' neighbours*/
    axiom_node_id_t neighbour_table[2][AXIOM_NODES_MAX];

    axiom_node_id_t node_index;
    axiom_if_id_t interface_index;
    axiom_node_id_t first_neighbour_id, second_neighbour_id;
    uint8_t neighbours_counter;

    /* for each node of the network */
    for (actual_node_id = 0; actual_node_id < total_nodes; actual_node_id++)
    {
        /* Init 'actual_node_id' node routing table and first and second
           level 'actual_node_id' neighbours tables */
        axiom_init_routing_neighbour(&routing_tables[actual_node_id],
                neighbour_table);

        /* Compute the first level neighbours*/
        neighbours_counter = axiom_compute_neighbours_route(actual_node_id,
                                                            topology,
                                                            routing_tables,
                                                            neighbour_table);
       do
       {
            /* Compute the second level neighbours*/
           for (node_index = 0; node_index < total_nodes; node_index++)
           {
               if (neighbour_table[0][node_index] == AXIOM_NULL_NODE)
               {
                   continue;
               }
               first_neighbour_id = neighbour_table[0][node_index];

               /* compute the neighbours of 'neighbour_id' node */
               for (interface_index = 0; interface_index < AXIOM_INTERFACES_MAX;
                       interface_index++)
               {
                   if ((topology[first_neighbour_id][interface_index]
                               != AXIOM_NULL_NODE) &&
                           (topology[first_neighbour_id][interface_index]
                               != actual_node_id))
                   {
                       second_neighbour_id =
                           topology[first_neighbour_id][interface_index];

                       if (routing_tables[actual_node_id][second_neighbour_id]
                               == AXIOM_NULL_RT_INTERFACE)
                       {
                           /* memorize that 'second_neighbour_id' node can be reached
                              through the interace connected with 'first_neighbour_id'*/
                           routing_tables[actual_node_id][second_neighbour_id] =
                               routing_tables[actual_node_id][first_neighbour_id];

                           /* save node id of the second level neighbour */
                           neighbour_table[1][second_neighbour_id] =
                               second_neighbour_id;

                           neighbours_counter++;
                       }
                   }
               }

           }

           /* the second level neighbours become the next first level
            * neighbours in the algorithm */
           memcpy(&neighbour_table[0], &neighbour_table[1],
                   AXIOM_NODES_MAX*sizeof(axiom_node_id_t));
       /* exit from cycle when 'actual_node_id' routing table has
        * been completed with all the AXIOM_NODES_MAX - 1 nodes info */
       } while (neighbours_counter < total_nodes - 1);
    }
}


/************************ Routing table delivery ******************************/

static void
axiom_init_routing_table(axiom_if_id_t routing_table[AXIOM_NODES_MAX])
{
    uint8_t i;

    for (i = 0; i < AXIOM_NODES_MAX; i++)
    {
        routing_table[i] = AXIOM_NULL_RT_INTERFACE;
    }
}

axiom_err_t
axiom_delivery_routing_tables(axiom_dev_t *dev,
        axiom_if_id_t routing_tables[][AXIOM_NODES_MAX],
        axiom_node_id_t total_nodes)
{
    axiom_node_id_t dest_node_index, rt_node_index;
    axiom_if_id_t ifaces;
    axiom_msg_id_t ret = AXIOM_RET_OK;

    /* MASTER: for each node */
    for (dest_node_index = AXIOM_MASTER_ID+1;
         (dest_node_index < total_nodes) && (ret == AXIOM_RET_OK);
         dest_node_index++)
    {
        /* send the routing table using raw messages */
        for (rt_node_index = 0; rt_node_index < total_nodes;
                rt_node_index++)
        {
            /* Decode the interfaces between dest_node_index and rt_node_index
             * (they are coded inside the routing table!)*/
            ifaces = routing_tables[dest_node_index][rt_node_index];
            if (ifaces != AXIOM_NULL_RT_INTERFACE)
            {
                 /* (node, ifaces codified) to send to dest_node_id*/
                /* (node, if) to send to dest_node_id*/
                ret = axiom_send_small_delivery(dev, dest_node_index,
                        AXIOM_RT_CMD_INFO, rt_node_index, ifaces);
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("MASTER, Error sending AXIOM_RT_TYPE_INFO message "
                            "to node %d", dest_node_index);
                    return ret;
                }
            }
        }
    }

    /* all routing table delivered; Master says: "end of delivery phase" */
    for (dest_node_index = AXIOM_MASTER_ID+1;
            (dest_node_index < total_nodes) && (ret == AXIOM_RET_OK);
            dest_node_index++)
	{
        /* end of sending routing tables */
        ret = axiom_send_small_delivery(dev, dest_node_index,
                AXIOM_RT_CMD_END_INFO, 0, 0);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("MASTER, Error sending AXIOM_RT_TYPE_END_INFO message "
                    "to node %d", dest_node_index);
            return ret;
        }
    }

    return ret;
}

axiom_err_t
axiom_wait_rt_received(axiom_dev_t *dev, axiom_node_id_t total_nodes)
{
    axiom_routing_cmd_t cmd = 0;
    axiom_node_id_t payload_node_id;
    axiom_node_id_t reply_received[AXIOM_NODES_MAX];
    axiom_if_id_t payload_if_id;
    int reply_received_num = 0;
    axiom_err_t ret;

    memset(reply_received, 0, sizeof(reply_received));

    while (reply_received_num != (total_nodes -1))
    {
        axiom_node_id_t src_node_id;

        /* receive reply from node which have received the routing table */
        ret = axiom_recv_small_delivery(dev, &src_node_id,
                &cmd, &payload_node_id, &payload_if_id);
        if ((ret != AXIOM_RET_OK) || (cmd != AXIOM_RT_CMD_RT_REPLY))
        {
            EPRINTF("MASTER, Error receiving AXIOM_RT_CMD_RT_REPLY message");
            EPRINTF("%d, %d", ret, cmd);
            return AXIOM_RET_ERROR;
        }

        if (reply_received[payload_node_id] == 0)
        {
            /* a 'new node' reply */
            reply_received[payload_node_id] = 1;
            reply_received_num++;
            DPRINTF("MASTER, received reply of node %d", payload_node_id);
        }
    }

    return AXIOM_RET_OK;
}

axiom_err_t
axiom_receive_routing_tables(axiom_dev_t *dev, axiom_node_id_t node_id,
        axiom_if_id_t routing_table[AXIOM_NODES_MAX],
        axiom_node_id_t *max_node_id)
{
    axiom_node_id_t src_node_id, node_to_set;
    axiom_if_id_t if_to_set;
    axiom_routing_cmd_t cmd = 0;
    axiom_node_id_t max_id = 0;
    axiom_err_t ret;

    axiom_init_routing_table(routing_table);

    while (cmd != AXIOM_RT_CMD_END_INFO)
    {
        /* receive routing info with raw messages */
        ret = axiom_recv_small_delivery(dev, &src_node_id,
                &cmd, &node_to_set, &if_to_set);

        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("Slave %d, Error receiving AXIOM_RT_TYPE_INFO message",
                    node_id);
            return ret;
        }
        if (cmd  == AXIOM_RT_CMD_INFO)
        {
            /* it is for local routing table, memorize it */
            routing_table[node_to_set] = if_to_set;

            /* compute the maximum node id of the network*/
            if (max_id < node_to_set)
            {
                max_id = node_to_set;
            }
        }
        else if (cmd == AXIOM_RT_CMD_END_INFO)
        {
            /* Master has finished with routing table delivery */

            /* compute the nodes total number of the network */
            if (node_id > max_id)
            {
                *max_node_id = node_id;
            }
            else
            {
                *max_node_id = max_id;
            }

            /* reply to MASTER that I'have received local routing table */
            ret = axiom_send_small_delivery(dev, AXIOM_MASTER_ID,
                    AXIOM_RT_CMD_RT_REPLY, node_id, 0);

            if (ret == AXIOM_RET_ERROR)
            {
                EPRINTF("Slave %d, Error sending AXIOM_RT_CMD_RT_REPLY message",
                        node_id);
                return ret;
            }
        }
    }
    return ret;
}


/*************************** Routing table set ********************************/

static void
axiom_write_routing_table(axiom_dev_t *dev,
        axiom_if_id_t routing_table[AXIOM_NODES_MAX])
{
    axiom_node_id_t node_id_index;

    /* set final routing table */
    for (node_id_index = 0; node_id_index < AXIOM_NODES_MAX; node_id_index++)
    {
        uint8_t enabled_mask = 0;
        enabled_mask = routing_table[node_id_index];
        if (enabled_mask != 0)
        {
            /* only if the node is connected to me
               I update local routing table */
            axiom_set_routing(dev, node_id_index, enabled_mask);
        }
    }

}

axiom_err_t
axiom_set_routing_table(axiom_dev_t *dev,
        axiom_if_id_t routing_table[AXIOM_NODES_MAX])
{
    axiom_if_id_t if_index;
    uint8_t if_features = 0;
    axiom_routing_cmd_t cmd = 0;
    axiom_if_id_t src_interface;
    axiom_if_id_t num_interface = 0;
    axiom_err_t ret = AXIOM_RET_OK;


    /* The node reads its node id */
    axiom_node_id_t node_id = axiom_get_node_id(dev);

    if (node_id == AXIOM_MASTER_ID)
    {
        /* ********************** Master node ******************** */

        /* Set its routing table */
        axiom_write_routing_table(dev, routing_table);

        /* Send message to all neighbours
        indicating to set their routing tables */

        /* For each interface send all messages */
        for (if_index = 0; (if_index < AXIOM_INTERFACES_MAX) &&
                (ret == AXIOM_RET_OK); if_index++)
        {
            /* get interface features */
            axiom_get_if_info (dev, if_index, &if_features);

            /* Interface connected to another board*/
            if ((if_features & 0x01) != 0)
            {
                DPRINTF("Node:%d, Send to interface number = %d the "
                        "AXIOM_RAW_TYPE_SET_ROUTING message",
                        node_id, if_index);

                /* Say over interface 'if_index': Set your routing table */
                ret = axiom_send_small_set_routing(dev, if_index,
                        AXIOM_RT_CMD_SET_ROUTING);
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("Node:%d, error sending to interface number = %d "
                            "the AXIOM_RAW_TYPE_SET_ROUTING message",
                            node_id, if_index);
                    return ret;
                }
            }
        }

        /* For each interface receive all messages */
        for (if_index = 0; (if_index < AXIOM_INTERFACES_MAX) &&
                (ret == AXIOM_RET_OK); if_index++)
        {
            /* get interface features */
            axiom_get_if_info (dev, if_index, &if_features);

            /* Interface connected to another board*/
            if ((if_features & 0x01) != 0)
            {
                /* Wait answers */
                ret = axiom_recv_small_set_routing(dev, &src_interface, &cmd);
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("Node:%d, error receiving AXIOM_RAW_TYPE_SET_ROUTING message",
                            node_id);
                    return ret;
                }
            }
        }

    }
    else
    {
        /* ********************* Slave nodes ********************* */

        /* wait for the first message saying to set the table */
        while ((cmd != AXIOM_RT_CMD_SET_ROUTING) && (ret == AXIOM_RET_OK))
        {
            DPRINTF("Node %d: Wait for AXIOM_RAW_TYPE_SET_ROUTING message",
                    node_id);

            ret = axiom_recv_small_set_routing(dev, &src_interface, &cmd);
            if (ret == AXIOM_RET_ERROR)
            {
                EPRINTF("Node:%d, error receiving AXIOM_RAW_TYPE_SET_ROUTING message",
                        node_id);
                return ret;
            }
            DPRINTF("Node %d: recevied from local interface %d:", node_id,
                    src_interface);

            if (cmd == AXIOM_RT_CMD_SET_ROUTING)
            {
                /* The first request to set the routing table */
                axiom_write_routing_table(dev, routing_table);
            }
        }

        /* For each interface send all messages */
        for (if_index = 0; (if_index < AXIOM_INTERFACES_MAX) &&
                (ret == AXIOM_RET_OK); if_index++)
        {
            /* get interface features */
            axiom_get_if_info (dev, if_index, &if_features);

            /* Interface connected to another board */
            if ((if_features & 0x01) != 0)
            {
                DPRINTF("Node:%d, Send to interface number = %d "
                        "the AXIOM_RAW_TYPE_SET_ROUTING message",
                        node_id, if_index);

                /* Say over interface 'if_index': Set your routing table */
                ret = axiom_send_small_set_routing(dev, if_index,
                        AXIOM_RT_CMD_SET_ROUTING);
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("Node:%d, error sending AXIOM_RAW_TYPE_SET_ROUTING message",
                            node_id);
                    return ret;
                }
                num_interface++;
            }
        }

        /* For each interface, (except the one from which I've
            already received) I receive messages, but I don't
            set the routing table (already set!!) */
        while ((num_interface > 1) && (ret == AXIOM_RET_OK))
        {
            /* Wait answer */
            ret = axiom_recv_small_set_routing(dev, &src_interface, &cmd);
            if (ret == AXIOM_RET_ERROR)
            {
                EPRINTF("Node:%d, axiom_recv_raw_set_routing() error",
                        node_id);
                return ret;
            }
            num_interface --;
        }
    }

    return ret;
}
