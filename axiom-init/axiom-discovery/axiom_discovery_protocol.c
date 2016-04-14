/*
 * axiom_discovery_protocol.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-22
 *
 * This file implements AXIOM NIC discovery phase
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>


#include "axiom_discovery_protocol.h"
#include "axiom_nic_discovery.h"

/********************************************************/
/*************** Discovery Algorithm Code ***************/
/********************************************************/

/* Initializes the gloabl Topology matrix of a node:
 * initially, it kwnows that no node is connected */
static void init_topology_structure(axiom_node_id_t topology[][AXIOM_MAX_INTERFACES])
{
    int i,j;

    for (i = 0; i < AXIOM_MAX_NODES; i++)
    {
        for (j = 0; j < AXIOM_MAX_INTERFACES; j++)
        {
            topology[i][j] = AXIOM_NULL_NODE; /* node id NULL_NODE doesn't exist*/
        }
    }
}

/* Codify the mask to set into the routing table */
uint8_t axiom_codify_routing_mask(axiom_dev_t *dev, axiom_node_id_t node_id,
                            uint8_t interface_index)
{
    uint8_t b_mask;

    axiom_get_routing(dev, node_id, &b_mask);
    b_mask |= (uint8_t)(1 << interface_index);

    return b_mask;
}

/*
 * @brief This function implements the Master and Slave common part
 *          of the discover algorithm.
 * @param dev The axiom device private data pointer
 * @param next_id The id returned from the node which ends its
 *                discovery phase; this id is the id of the first
 *                node that has finished its discovery phase after
 *                the node that is exiting from this function
 *                was sent "START DISCOVERY" message to its neighbour.
 * @param topology matrix memorizing the each node partial topology.
 *          When all nodes have executed this function, the topology
 *          matrix of Master contatins the global topology of the
 *          network.
 *
 */
static int discover_phase(axiom_dev_t *dev, axiom_node_id_t *next_id,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES])
{
    axiom_if_id_t num_interface = 0;
    uint8_t if_features = 0;
    axiom_discovery_cmd_t msg_cmd = 0;
    axiom_node_id_t src_node_id, dst_node_id;
    axiom_if_id_t src_interface;
    axiom_if_id_t data_src_if, data_dst_if;
    axiom_node_id_t start_node_index, node_index;
    int i, a, b;
    uint8_t b_mask;
    axiom_msg_id_t ret;

    /* The node reads its node id (1) */
    axiom_node_id_t my_node_id = axiom_get_node_id(dev);

    DPRINTF("my_node_id = %d", my_node_id);

    /* The node reads the info about the interfaces */
    axiom_get_if_number(dev, &num_interface);

    DPRINTF("Number of interface = %d", num_interface);

    /* For each interface */
    for (i = 0; i < AXIOM_MAX_INTERFACES; i++)
    {
        /* get interface feature*/
        axiom_get_if_info (dev, i, &if_features);

        if ((if_features & AXIOM_IF_CONNECTED) != 0) /* the interface 'i' is physically connected to another board*/
        {
            if (topology[my_node_id][i] != AXIOM_NULL_NODE)
            {
                /*     The node has to poll the interface only if the topology table
                       indicates that on the interface, the neighour node id is unknown.
                       Infact if the node on that interface has already un ID and node s
                       ends it a message, node will be blocked waiting a response that
                       the neighbour (that has alreay ended its protocol) will never send it.
                */
                ;
            }
            else
            {
                DPRINTF("Node:%d, Send to interface number = %d the AXIOM_DSCV_CMD_REQ_ID message", my_node_id, i);

                /* Say over interface 'i': 'I am node my_node_id give me your node id' */          
                ret = axiom_send_small_discovery(dev, i, AXIOM_DSCV_CMD_REQ_ID, my_node_id, 0, 0, 0);
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("Node:%d, Error sending to interface number = %d the AXIOM_DSCV_CMD_REQ_ID message", my_node_id, i);
                }
                else
                {

                    DPRINTF("Node:%d, Wait for receiving response on interface number = %d", my_node_id, i);

                    /* Wait for the neighbour response */
                    ret = axiom_recv_small_discovery(dev, &msg_cmd,
                                               &src_node_id, &dst_node_id,
                                               &src_interface,
                                               &data_src_if, &data_dst_if);

                    if (ret == AXIOM_RET_ERROR)
                    {
                        EPRINTF("Node:%d, Error receiving response on interface number = %d", my_node_id, i);
                    }
                    else
                    {
                        /* message received form the 'i' interface and it is the exepected message */
                        if ((msg_cmd == AXIOM_DSCV_CMD_RSP_NOID) || (msg_cmd == AXIOM_DSCV_CMD_RSP_ID))
                        {
                            if (msg_cmd == AXIOM_DSCV_CMD_RSP_NOID)
                            {
                                DPRINTF("Node:%d, Received from interface %d the message AXIOM_DSCV_CMD_RSP_NOID", my_node_id, i);

                                /* if the receiving side says 'I do not know, I'm on interface Y' */
                                (*next_id)++;

                                DPRINTF("Node:%d, Send to interface number = %d the AXIOM_DSCV_CMD_SETID message, id_node=%d", my_node_id, i, *next_id);

                                /* Say over interface 'i': you are node 'nextid' */
                                ret = axiom_send_small_discovery(dev, i, AXIOM_DSCV_CMD_SETID, my_node_id, *next_id, 0, 0);
                                if (ret == AXIOM_RET_ERROR)
                                {
                                    EPRINTF("Node:%d, Error sending to interface number = %d the AXIOM_DSCV_CMD_SETID message, id_node=%d", my_node_id, i, *next_id);
                                }
                                else
                                {
                                    /* Immediately update my routing table: next_id node is connceted to my 'i' interface */
                                    b_mask = axiom_codify_routing_mask(dev, *next_id, i);
                                    axiom_set_routing(dev, *next_id, b_mask);

                                    /* Update the topology data structure:  my 'i' interface is connected  the new next_id node
                                       and next_id node is connected to me */
                                    topology[my_node_id][i] = *next_id;
                                    topology[*next_id][data_src_if] = my_node_id;

                                    DPRINTF("Node:%d, Update my topology structure:", my_node_id);
                                    DPRINTF("Node:%d If:%d Connected to Node:%d", my_node_id, i, *next_id);
                                    DPRINTF("Node:%d If:%d Connected to _node:%d", *next_id, data_src_if, my_node_id);

                                    DPRINTF("Node:%d, Send to interface number = %d the AXIOM_DSCV_CMD_START message", my_node_id, i);
                                    /* Say over interface 'i': start discovery protocol, nextid */
                                    ret = axiom_send_small_discovery(dev, i, AXIOM_DSCV_CMD_START, my_node_id, *next_id, 0, 0);
                                    if (ret == AXIOM_RET_ERROR)
                                    {
                                        EPRINTF("Node:%d, Error sending to interface number = %d the AXIOM_DSCV_CMD_START message", my_node_id, i);
                                    }
                                    else
                                    {
                                        start_node_index = *next_id;

                                        msg_cmd = 0;
                                        /* wait for the messages with the next_id topology data structure */
                                        while (msg_cmd != AXIOM_DSCV_CMD_END_TOPOLOGY)
                                        {
                                            DPRINTF("Node:%d, Wait for AXIOM_DSCV_CMD_END_TOPOLOGY/AXIOM_DSCV_CMD_TOPOLOGY messages", my_node_id);

                                            /* Receive back the  next_id neighbour node topology data structure and the new updated nextid
                                               or an ID request */
                                            ret = axiom_recv_small_discovery(dev, &msg_cmd,
                                                                       &src_node_id, &dst_node_id,
                                                                       &src_interface,
                                                                       &data_src_if, &data_dst_if);
                                            if (ret == AXIOM_RET_ERROR)
                                            {
                                                EPRINTF("Node:%d, Error in receiving while waiting for AXIOM_DSCV_CMD_END_TOPOLOGY/AXIOM_DSCV_CMD_TOPOLOGY messages", my_node_id);
                                            }
                                            else
                                            {
                                                /* request for my id from a node which is executing its discovery algorithm */
                                                if (msg_cmd == AXIOM_DSCV_CMD_REQ_ID)
                                                {
                                                    /* Immediately update my routing table: src_node_id node is connceted to my 'src_interface' interface */
                                                    /* b_mask = axiom_codify_routing_mask(dev, src_node_id, src_interface);
                                                       axiom_set_routing(dev, src_node_id, b_mask); */

                                                    /* Reply 'I am node 'my_node_id', I'm on interface 'src_interface' */
                                                    ret = axiom_send_small_discovery(dev, src_interface,
                                                                            AXIOM_DSCV_CMD_RSP_ID,
                                                                            my_node_id, src_node_id, src_interface, 0);
                                                    DPRINTF("Node:%d, Send AXIOM_DSCV_CMD_RSP_ID on if %d",  my_node_id, src_interface);
                                                    if (ret == AXIOM_RET_ERROR)
                                                    {
                                                        EPRINTF("Node:%d, Error sending AXIOM_DSCV_CMD_RSP_ID messages", my_node_id);
                                                    }
                                                }
                                                /* request of memorizing topology info */
                                                else if (msg_cmd == AXIOM_DSCV_CMD_TOPOLOGY)
                                                {
                                                    /* Update the topology data structure */
                                                    topology[src_node_id][data_src_if] = dst_node_id;
                                                    topology[dst_node_id][data_dst_if] = src_node_id;

                                                    DPRINTF("Node:%d, RECEIVE_TOPOLOGY", my_node_id);
                                                    DPRINTF("%d %d Connected To %d", src_node_id, data_src_if, dst_node_id);
                                                    DPRINTF("%d %d Connected To %d", dst_node_id, data_dst_if, src_node_id);

                                                }
                                                else if (msg_cmd == AXIOM_DSCV_CMD_END_TOPOLOGY)
                                                {
                                                    DPRINTF("Node:%d, AXIOM_DSCV_CMD_END_TOPOLOGY messages received", my_node_id);

                                                    /* get the updated nextid */
                                                    *next_id = src_node_id;

                                                    /* Update routing table of all nodes discovered by the node to which
                                                                 I request to execute the discovery algorithm */
                                                    for (node_index = start_node_index; node_index <= (*next_id); node_index++)
                                                    {
                                                        b_mask = axiom_codify_routing_mask(dev, node_index, i);
                                                        axiom_set_routing(dev, node_index, b_mask);
                                                    }
                                                    NDPRINTF("Node:%d TOPOLOGY\n", my_node_id);
                                                    for (a = 0; a < AXIOM_MAX_NODES; a++)
                                                    {
                                                        for (b = 0; b < AXIOM_MAX_INTERFACES; b++)
                                                            NDPRINTF("%d ", topology[a][b]);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                /* if the receiving side says 'I am node X, I'm on interface Y' */

                                DPRINTF("Node:%d, Received from interface %d the message AXIOM_DSCV_CMD_RSP_ID", my_node_id, i);

                                /* Update the topology data structure */
                                topology[my_node_id][src_interface] = src_node_id;
                                topology[src_node_id][data_src_if] = my_node_id;

                                DPRINTF("Node:%d, Update my topology structure:", dst_node_id);
                                DPRINTF("Node:%d If:%d Connected to Node:%d", my_node_id, src_interface, src_node_id);
                                DPRINTF("Node:%d If:%d Connected to Node:%d", src_node_id, src_node_id, my_node_id);
                            }
                        }
                        else
                        {
                            EPRINTF("Node:%d, interface or msg type error", my_node_id);
                        }
                    }
                }
            }
        }
    }

    return AXIOM_RET_OK;
}

/* Master node Discovery Algorithm code */
int axiom_master_node_discovery(axiom_dev_t *dev,
                          axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
                          axiom_node_id_t *number_of_total_nodes)
{
    axiom_node_id_t next_id = AXIOM_MASTER_ID;
    int ret;

    init_topology_structure(topology);

    /* sets its id to zero */
    axiom_set_node_id(dev, AXIOM_MASTER_ID);

    ret = discover_phase(dev, &next_id, topology);
    *number_of_total_nodes = next_id + 1;

    return ret;
}

/* Other nodes Discovery Algorithm code */
int axiom_slave_node_discovery (axiom_dev_t *dev,
        axiom_node_id_t topology[][AXIOM_MAX_INTERFACES],
        axiom_node_id_t *my_node_id,
        axiom_if_id_t first_interface, axiom_payload_t first_msg)
{
    axiom_node_id_t next_id;
    axiom_discovery_cmd_t msg_cmd ;
    axiom_node_id_t src_node_id, dst_node_id;
    axiom_if_id_t src_interface, data_src_if, data_dst_if;
    int i, j, w;
    uint8_t b_mask;
    axiom_msg_id_t ret;

    /* called when received the first AXIOM_DSCV_CMD_REQ_ID type message */
    src_interface = first_interface;
    src_node_id = ((axiom_discovery_payload_t *) &first_msg)->src_node;
    msg_cmd = ((axiom_discovery_payload_t *) &first_msg)->command;

    if (msg_cmd != AXIOM_DSCV_CMD_REQ_ID)
    {
        EPRINTF("Slave: Expected AXIOM_DSCV_CMD_REQ_ID message");
        return AXIOM_RET_ERROR;
    }

    /* init local topolgy structure */
    init_topology_structure(topology);

    /* reset id when discovery phase starts */
    *my_node_id = 0;
    axiom_set_node_id(dev, *my_node_id);
    *my_node_id = axiom_get_node_id(dev);

    DPRINTF("Slave: my_node_id = %d", *my_node_id);

    /* Immediately update my routing table: src_node_id node is connceted to my 'src_interface' interface */
    b_mask = axiom_codify_routing_mask(dev, src_node_id, src_interface);
    axiom_set_routing(dev, src_node_id, b_mask);

    /* If I do not have a node _id */
    if (*my_node_id == 0)
    {
        DPRINTF("Slave: send to interface number = %d the AXIOM_DSCV_CMD_RSP_NOID message", src_interface);

        /* Reply 'I do not have an id, I'm on interface src_interface' */
        ret = axiom_send_small_discovery(dev, src_interface, AXIOM_DSCV_CMD_RSP_NOID, *my_node_id, src_node_id, src_interface, 0);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("Slave: error sending to interface number = %d the AXIOM_DSCV_CMD_RSP_NOID message", src_interface);
        }
        else
        {
            DPRINTF("Slave: Wait for AXIOM_DSCV_CMD_SETID message");

            /* Wait for the neighbour AXIOM_DSCV_CMD_SETID type message */
            ret = AXIOM_RET_OK;
            while ((msg_cmd != AXIOM_DSCV_CMD_SETID) && (ret == AXIOM_RET_OK))
            {
                ret = axiom_recv_small_discovery(dev, &msg_cmd,
                                           &src_node_id, &dst_node_id,
                                           &src_interface,
                                           &data_src_if, &data_dst_if);
            }
            if (ret == AXIOM_RET_ERROR)
            {
                EPRINTF("Slave: Error receving AXIOM_DSCV_CMD_SETID message, my id =%d", dst_node_id);
            }
            else
            {
                DPRINTF("Slave: Received AXIOM_DSCV_CMD_SETID message, my id =%d", dst_node_id);

                /* set my id*/
                axiom_set_node_id(dev, dst_node_id);
                *my_node_id = dst_node_id;

                DPRINTF("Slave: Wait for AXIOM_DSCV_CMD_START message");

                /* Wait for the neighbour AXIOM_DSCV_CMD_START type message */
                ret = AXIOM_RET_OK;
                while ((msg_cmd != AXIOM_DSCV_CMD_START) && (ret == AXIOM_RET_OK))
                {
                    ret = axiom_recv_small_discovery(dev, &msg_cmd,
                                               &src_node_id, &dst_node_id,
                                               &src_interface,
                                               &data_src_if, &data_dst_if);
                }
                if (ret == AXIOM_RET_ERROR)
                {
                    EPRINTF("Slave id:%d Error receiving AXIOM_DSCV_CMD_START message", *my_node_id);
                }
                else
                {
                    next_id = dst_node_id; /* that is my id, previously yet recevied into data field! */

                    DPRINTF("Slave id:%d Received AXIOM_DSCV_CMD_START message, I start discovery phase ", *my_node_id);

                    /* Execute the dicovery algorithm*/
                    discover_phase(dev, &next_id, topology);

                    DPRINTF("Node:%d Ended discovery phase, send my topology table to _node:%d on my interface:%d", *my_node_id, src_node_id, src_interface);

                    /* Send topology list to the node which sent me the start message */
                    for (i = 0; i < AXIOM_MAX_NODES; i ++)
                    {
                        for (j = 0; j < AXIOM_MAX_INTERFACES; j++)
                        {
                            if (topology[i][j] != AXIOM_NULL_NODE)
                            {
                                /* find the remote node interface */
                                for(w = 0; w < AXIOM_MAX_INTERFACES; w++)
                                {
                                    if (topology[topology[i][j]][w] == i)
                                        break;
                                }
                                /* there is a link to return */
                                ret = axiom_send_small_discovery(dev, src_interface, AXIOM_DSCV_CMD_TOPOLOGY,
                                                     i, topology[i][j],
                                                     j, w);
                                if (ret == AXIOM_RET_ERROR)
                                {
                                    EPRINTF("Slave id:%d Error receiving AXIOM_DSCV_CMD_START message", *my_node_id);
                                }
                                else
                                {
                                    DPRINTF("%d %d %d %d",i, j, topology[i][j], w);
                                }
                            }
                        }
                    }

                    DPRINTF("Node:%d Send AXIOM_DSCV_CMD_END_TOPOLOGY message to Node:%d on my interface:%d", *my_node_id, src_node_id, src_interface);

                    /* Says: <<Finished sending the topology structure, I send back actual next_id>> */
                    ret = axiom_send_small_discovery(dev, src_interface, AXIOM_DSCV_CMD_END_TOPOLOGY, next_id, 0, 0, 0);
                    if (ret == AXIOM_RET_ERROR)
                    {
                        EPRINTF("Slave id:%d Error sending AXIOM_DSCV_CMD_END_TOPOLOGY message", *my_node_id);
                    }
                }
            }
        }
    }
    else
    {
        /* Reply 'I am node 'my_node_id', I'm on interface 'src_interface' */
        ret = axiom_send_small_discovery(dev, src_interface, AXIOM_DSCV_CMD_RSP_ID, *my_node_id, src_node_id, src_interface, 0);
        if (ret == AXIOM_RET_ERROR)
        {
            EPRINTF("Slave id:%d Error sending AXIOM_DSCV_CMD_RSP_ID message", *my_node_id);
        }
    }

    return ret;
}

