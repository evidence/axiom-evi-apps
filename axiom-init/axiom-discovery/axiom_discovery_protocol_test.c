/*
 * axiom_discovery_protocol_test.c
 *
 * Version:     v0.3.1
 * Last update: 2016-03-23
 *
 * This file tests the AXIOM INIT implementation
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_discovery_node.h"
#include "axiom_simulator.h"
#include "axiom_net.h"

axiom_sim_node_args_t axiom_nodes[AXIOM_MAX_NODES];

axiom_sim_topology_t final_topology = {
    .num_nodes = AXIOM_MAX_NODES,
    .num_interfaces = AXIOM_MAX_INTERFACES
};

extern axiom_node_id_t topology[AXIOM_MAX_NODES][AXIOM_MAX_INTERFACES];

static void usage(void)
{
    printf("usage: ./axiom_discovery_protocol |  ./axsw_discovery_protocol \n");
    printf ("                           [[-f file_name] | [-h]] \n\n");
    printf("-f, --file      file_name           toplogy file \n");
    printf("-h, --help                          print this help\n");
}

/* Initializes start_toplogy with the file input topology
   and returns the number of nodes into */
static int
axiom_topology_from_file(axiom_topology_t *start_topology, char *filename) {

    FILE *file = NULL;
    char *line = NULL, *p;
    size_t len = 0;
    ssize_t read;
    int line_count = 0, if_index = 0;

    file = fopen(filename, "r");
    if (file == NULL)  {
        printf ("File not existent \n");
        return -1;
    }

    while ((read = getline(&line, &len, file)) != -1) {
       printf("%s", line);

       line_count++;
       if (line_count > AXIOM_MAX_NODES) {
           printf ("The topology contains more than %d nodes\n", AXIOM_MAX_NODES);
           return -1;
       }
       if_index = 0;
       p = line;
       /* While there are characters into the line */
       while (*p) {
           if (isdigit(*p)) {
               /* Upon finding a digit, read it */
               long val = strtol(p, &p, 10);
               //printf("%ld\n", val);
               if ((val ==  LONG_MIN) || (val ==  LONG_MAX)) {
                   printf ("Error in converting nodes id read from file\n");
                   return -1;
               }
               else
               {
                   if ((val < 0) || (val > AXIOM_MAX_NODES)) {
                       printf ("The topology contains nodes with id greater than %d\n", AXIOM_MAX_NODES);
                       return -1;
                   }
                   else {
                       if (if_index >= AXIOM_MAX_INTERFACES) {
                           printf ("The topology contains nodes with more than  than %d interfaces\n", AXIOM_MAX_INTERFACES);
                           return -1;
                       }
                       else {
                           start_topology->topology[line_count-1][if_index] = (uint8_t)val;
                           printf ("logic->start_topology.topology[%d][%d] = %d\n", line_count-1, if_index, (uint8_t)val);
                           if_index++;
                       }
                   }
               }
           }
           else {
               /* move on to the next character of the line */
               p++;
           }
       }
    }
    start_topology->num_nodes = line_count;
    start_topology->num_interfaces = AXIOM_MAX_INTERFACES;
    free(line);
    fclose(file);

    return line_count;

}

/* Given the start_topology it computes the intermediate expected topology */
void axiom_compute_intermediate_final_topology(axiom_topology_t *start_topology,
                                 axiom_topology_t *end_test_topology, uint8_t *nodes_match,
                                 uint8_t start_node_id, uint8_t *max_node_id, int num_nodes)
{
    int iface;
    uint8_t node, old_id;

    for (iface = 0; iface < AXIOM_MAX_INTERFACES; iface++)
    {
        if (start_topology->topology[start_node_id][iface] != AXIOM_NULL_NODE)
        {
            node = nodes_match[start_topology->topology[start_node_id][iface]];
            if (node != AXIOM_NULL_NODE)
            {
                end_test_topology->topology[start_node_id][iface] = node;
            }
            else
            {
                (*max_node_id) ++;
                /* memorize that new node id corresponds to start_node_id node */
                old_id = start_topology->topology[start_node_id][iface];
                nodes_match[old_id] = *max_node_id;
                end_test_topology->topology[start_node_id][iface] = *max_node_id;
                axiom_compute_intermediate_final_topology(start_topology, end_test_topology,
                                                          nodes_match,
                                                          old_id,  max_node_id, num_nodes);
            }
        }
    }
}

/* Given the start_topology it computes the final expected topology */
void axiom_compute_final_topology(axiom_topology_t *start_topology,
                                 axiom_topology_t *end_test_topology, uint8_t *nodes_match,
                                 uint8_t start_node_id, uint8_t *max_node_id, int num_nodes,
                                 axiom_topology_t *end_test_topology_copy)
{
    int i;

    axiom_compute_intermediate_final_topology(start_topology, end_test_topology,
                                nodes_match, start_node_id, max_node_id, num_nodes);

    memcpy(end_test_topology_copy, end_test_topology,
           sizeof(axiom_topology_t));

    for (i = 0; i < num_nodes; i++)
    {
        memcpy(end_test_topology->topology[nodes_match[i]],
               end_test_topology_copy->topology[i],
                sizeof(uint8_t) * AXIOM_MAX_INTERFACES);
    }
}

void *master(void *args)
{
    axiom_sim_node_args_t *axiom_args = (axiom_sim_node_args_t *) args;
    axiom_dev_t *p_node_info;

    set_node_info(axiom_args);
    sleep(1);
    NDPRINTF("axiom_args %p get_node_info %p", axiom_args, get_node_info());

    /* get the pointer to the running thread 'axiom_nodes' entry */
    p_node_info = (axiom_dev_t *)get_node_info();

    axiom_discovery_master(p_node_info, axiom_args->node_topology, axiom_args->routing_tables,
					axiom_args->final_routing_table, 1);

    memcpy(final_topology.topology, axiom_args->node_topology, sizeof(axiom_args->node_topology));

    DPRINTF("END");


    return 0;
}

void *slave(void *args)
{
    axiom_sim_node_args_t *axiom_args = (axiom_sim_node_args_t *) args;
    axiom_dev_t *p_node_info;

    set_node_info(axiom_args);
    sleep(1);
    NDPRINTF("axiom_args %p get_node_info %p", axiom_args, get_node_info());

    /* get the pointer to the running thread 'axiom_nodes' entry */
    p_node_info = (axiom_dev_t *)get_node_info();
    axiom_discovery_slave(p_node_info, axiom_args->node_topology,
            axiom_args->final_routing_table, 1);

    return 0;
}

int main(int argc, char **argv)
{
    int err, cmp;
    int i, j;
    int file_ok = 0;
    int long_index =0;
    int opt = 0, num_nodes;
    uint8_t max_node_id = 0;
    uint8_t nodes_match[AXIOM_MAX_NODES];
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"topology", required_argument, 0, 't'},
        {"n", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    char filename[100];
    axiom_topology_t start_topology;
    axiom_topology_t end_test_topology, end_test_topology_copy;

    while ((opt = getopt_long(argc, argv,"f:t:n:h",
                         long_options, &long_index )) != -1) {
        switch (opt) {
            case 'f' :
                if (sscanf(optarg, "%s", filename) != 1) {
                    usage();
                    exit(-1);
                }
                else {
                    file_ok = 1;
                }
                break;

            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    /* check file presence */
    if (file_ok == 1) {
        /* init the topology structure */
        num_nodes = axiom_topology_from_file(&start_topology, filename);
        if (num_nodes < 0)
        {
            printf("Error in reading toplogy from file\n");
            exit(-1);
        }
        else
        {
            /* Initilaization of variable for ended toplogy computataion */
            for (i = 0; i < AXIOM_MAX_NODES; i++)
            {
                nodes_match[i] = AXIOM_NULL_NODE;
                for (j = 0; j < AXIOM_MAX_INTERFACES; j++)
                {
                    end_test_topology.topology[i][j] = AXIOM_NULL_NODE;
                }
            }
            nodes_match[0] = 0;

            /* *********** Expected final topology computation *************** */
            axiom_compute_final_topology (&start_topology, &end_test_topology,
                                          nodes_match, 0, &max_node_id, num_nodes,
                                          &end_test_topology_copy);
        }
    }
    else {
       usage();
       exit(-1);
    }

    printf("Start Topology\n");
    print_topology(&start_topology);

    /* initilize links between nodes */
    err = axiom_net_setup(axiom_nodes, &start_topology);
    if (err) {
        return err;
    }

    /* start nodes threads */
    start_nodes(axiom_nodes, num_nodes, AXIOM_MASTER_ID, master, slave);


    /* wait nodes */
    wait_nodes(axiom_nodes, num_nodes);

    /* ********************************************************************* */
    /* ************************* TEST RESULTS ****************************** */
    /* ********************************************************************* */
    printf("\n************* TEST RESULTS *******************");
    /* ************ TOPOLOGY *****************/
    cmp = memcmp(&end_test_topology.topology, &final_topology.topology,
                  sizeof(uint8_t) * AXIOM_MAX_NODES * AXIOM_MAX_INTERFACES);
    if (cmp == 0)
    {
        printf("\nDiscovered Topology OK\n");
    }
    else
    {
        printf("\nDiscovered Topology ERROR\n");
    }

    /* ********************************************************************* */
    /* *********************** END TEST RESULTS **************************** */
    /* ********************************************************************* */

    axiom_net_free(axiom_nodes, &start_topology);

    return 0;
}
