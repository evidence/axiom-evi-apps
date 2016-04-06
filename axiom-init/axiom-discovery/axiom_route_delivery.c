/*
 * axiom_route_delivery.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-22
 *
 * This file implements routing table delivery
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_discovery_protocol.h"
#include "axiom_route_delivery.h"
#include "axiom_route_compute.h"
#include "axiom_nic_routing.h"

/*
 * @brief This function is executed by each node to initialize
 *        its local routing table before receiving it from
 *        Master
 * @param final_routing_table node local routing table to initialize
 */
static void init_my_routing_table (axiom_if_id_t final_routing_table[AXIOM_MAX_NODES])
{
    uint8_t i;

    for (i = 0; i < AXIOM_MAX_NODES; i++)
    {
        /* initialized the zero; it will be written with coded
           interface values:
           (AXIOM_IF_0, AXIOM_IF_1, AXIOM_IF_2, AXIOM_IF_3) */
        final_routing_table[i] = AXIOM_NULL_RT_INTERFACE;
    }
}

axiom_msg_id_t axiom_delivery_routing_tables(axiom_dev_t *dev,
                           axiom_if_id_t routing_tables[][AXIOM_MAX_NODES],
                           axiom_node_id_t number_of_total_nodes)
{
    axiom_node_id_t dest_node_index, rt_node_index;
    axiom_if_id_t ifaces;
    axiom_msg_id_t ret = AXIOM_RET_OK;

    /* MASTER: for each node */
    for (dest_node_index = AXIOM_MASTER_ID+1;
         (dest_node_index < number_of_total_nodes) && (ret == AXIOM_RET_OK);
         dest_node_index++)
    {
        /* send the routing table using raw messages */
        for (rt_node_index = 0; rt_node_index < number_of_total_nodes; rt_node_index++)
        {
            /* Decode the interfaces between
             * dest_node_index and rt_node_index
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
                    EPRINTF("MASTER, Error sending AXIOM_RT_TYPE_INFO message to node %d", dest_node_index);
                }
            }
        }
    }

    /* all routing table delivered; Master says: "end of delivery phase" */
    for (dest_node_index = AXIOM_MASTER_ID+1; (dest_node_index < number_of_total_nodes) && (ret == AXIOM_RET_OK); dest_node_index++)
	{
        /* end of sending routing tables */
        ret = axiom_send_small_delivery(dev, dest_node_index,
                AXIOM_RT_CMD_END_INFO, 0, 0);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("MASTER, Error sending AXIOM_RT_TYPE_END_INFO message to node %d", dest_node_index);
        }
    }

    return ret;
}

axiom_err_t axiom_receive_routing_tables(axiom_dev_t *dev,
                        axiom_node_id_t my_node_id,
                        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES],
                        axiom_node_id_t *max_node_id)
{
    axiom_node_id_t src_node_id, node_to_set;
    axiom_if_id_t if_to_set;
    axiom_routing_cmd_t cmd = 0;
    axiom_node_id_t max_id = 0;
    axiom_err_t ret;

    init_my_routing_table(final_routing_table);

    while (cmd != AXIOM_RT_CMD_END_INFO)
    {
        /* receive routing info with raw messages */
        ret = axiom_recv_small_delivery(dev, &src_node_id,
                &cmd, &node_to_set, &if_to_set);

        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("Slave %d, Error receiving AXIOM_RT_TYPE_INFO message", my_node_id);
        }
        else
        {
            if (cmd  == AXIOM_RT_CMD_INFO)
            {
                /* it is for my routing table, memorize it */
                final_routing_table[node_to_set] = if_to_set;

                /* compute the maximum node id of the network*/
                if (max_id < node_to_set)
                {
                    max_id = node_to_set;
                }
            }
            else if (cmd == AXIOM_RT_CMD_END_INFO)
            {
                /* Master has finished with routing table delivery,
                 * wait for the raw neighbour message to effectively
                 * update my routing table */
                if (my_node_id > max_id)
                {
                    *max_node_id = my_node_id;
                }
                else
                {
                    *max_node_id = max_id;
                }
            }
        }
    }
    return ret;

    /* wait for neighbour message for setting my routing table */

}





























