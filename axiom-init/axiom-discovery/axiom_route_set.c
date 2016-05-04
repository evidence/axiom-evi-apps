/*
 * This file implements AXIOM NIC final routing table set
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_discovery_protocol.h"
#include "axiom_route_compute.h"
#include "axiom_route_set.h"
#include "axiom_nic_routing.h"

/*
 * @brief This function writes the local node routing table
 * @param dev The axiom device private data pointer
 * @param final_routing_table local routing table previously
 *                            received from Master node
 */
static void
set_routing_table(axiom_dev_t *dev,
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES])
{
    axiom_node_id_t node_id_index;

    /* set final routing table */
    for (node_id_index = 0; node_id_index < AXIOM_MAX_NODES; node_id_index++)
    {
        uint8_t enabled_mask = 0;
        enabled_mask = final_routing_table[node_id_index];
        if (enabled_mask != 0)
        {
            /* only if the node is connected to me
               I update local routing table */
            axiom_set_routing(dev, node_id_index, enabled_mask);
        }
    }

}

axiom_msg_id_t
axiom_set_routing_table(axiom_dev_t *dev,
        axiom_if_id_t final_routing_table[AXIOM_MAX_NODES])
{
    axiom_if_id_t if_index;
    uint8_t if_features = 0;
    axiom_routing_cmd_t cmd = 0;
    axiom_if_id_t src_interface;
    axiom_if_id_t num_interface = 0;
    axiom_msg_id_t ret = AXIOM_RET_OK;


    /* The node reads its node id */
    axiom_node_id_t node_id = axiom_get_node_id(dev);

    if (node_id == AXIOM_MASTER_ID)
    {
        /* ********************** Master node ******************** */

        /* Set its routing table */
        set_routing_table(dev, final_routing_table);

        /* Send message to all neighbours
        indicating to set their routing tables */

        /* For each interface send all messages */
        for (if_index = 0; (if_index < AXIOM_MAX_INTERFACES) &&
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
        for (if_index = 0; (if_index < AXIOM_MAX_INTERFACES) &&
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
        /* ********************* Other nodes ********************* */

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
                set_routing_table(dev, final_routing_table);
            }
        }

        /* For each interface send all messages */
        for (if_index = 0; (if_index < AXIOM_MAX_INTERFACES) &&
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





























