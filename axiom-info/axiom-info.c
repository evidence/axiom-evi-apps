/*!
 * \file axiom-info.c
 *
 * \version     v0.5
 * \date        2016-05-03
 *
 * This file contains the implementation of axiom-info application.
 *
 * axiom-info prints information about the AXIOM NIC
 */
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dprintf.h"
#include "axiom_nic_types.h"
#include "axiom_nic_limits.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_regs.h"

#define PRINT_NODEID            0x0001
#define PRINT_IFNUMBER          0x0002
#define PRINT_IFINFO            0x0004
#define PRINT_ROUTING           0x0008
#define PRINT_ROUTING_ALL       0x0010
#define PRINT_STATUS            0x0020
#define PRINT_CONTROL           0x0040
#define PRINT_NUMNODES          0x0080

#define PRINT_ALL               0xFFFF

#define PRINT_ALL_RTSET         (PRINT_ALL & ~PRINT_ROUTING_ALL)

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-info [arguments]\n");
    printf("Print information about the AXIOM NIC\n\n");
    printf("Arguments:\n");
    printf("-a, --all (default)         print all information\n");
    printf("-n, --nodeid                print node id\n");
    printf("-i, --ifnumber              print number of interfaces\n");
    printf("-f, --ifinfo      if_id     print information of given interface (if_id)\n");
    printf("-N, --numnodes              print number of nodes in the network\n");
    printf("-r, --routing               print routing table (only reachable nodes)\n");
    printf("-R, --routing-all           print routing table (all nodes)\n");
    printf("-s, --status                print status register\n");
    printf("-c, --control               print control register\n");
    printf("-h, --help                  print this help\n\n");
}

static void
print_nodeid(axiom_dev_t *dev)
{
    axiom_node_id_t nodeid;

    nodeid = axiom_get_node_id(dev);

    printf("\tnode id = %u\n\n", nodeid);
}

static void
print_ifnumber(axiom_dev_t *dev)
{
    axiom_err_t err;
    axiom_if_id_t if_number;

    err = axiom_get_if_number(dev, &if_number);
    if (err) {
        EPRINTF("err: %x if_number: %x", err, if_number);
    }

    printf("\tnumber of interfaces = %u\n\n", if_number);
}

static void
print_ifinfo(axiom_dev_t *dev, axiom_if_id_t if_id)
{
    axiom_err_t err;
    uint8_t if_features;

    err = axiom_get_if_info(dev, if_id, &if_features);
    if (err) {
        EPRINTF("err: %x if_features: %x", err, if_features);
    }

    printf("\tinterfaces[%u] status: 0x%02x\n", if_id, if_features);

    if (if_features & AXIOMREG_IFINFO_CONNECTED) {
        printf("\t\tconnceted = 1\n");
    } else {
        printf("\t\tconnceted = 0\n");
    }

    if (if_features & AXIOMREG_IFINFO_TX) {
        printf("\t\ttx-enabled = 1\n");
    } else {
        printf("\t\ttx-enabled = 0\n");
    }

    if (if_features & AXIOMREG_IFINFO_RX) {
        printf("\t\trx-enabled = 1\n");
    } else {
        printf("\t\trx-enabled = 0\n");
    }
    printf("\n");
}

static void
print_ifinfo_all(axiom_dev_t *dev)
{
    axiom_err_t err;
    axiom_if_id_t if_number;
    int i;

    err = axiom_get_if_number(dev, &if_number);
    if (err) {
        EPRINTF("err: %x if_number: %x", err, if_number);
    }

    for (i = 0; i < if_number; i++) {
        print_ifinfo(dev, i);
    }
}

static void
print_num_nodes(axiom_dev_t *dev)
{
    int num_nodes;

    num_nodes = axiom_get_num_nodes(dev);
    if (num_nodes < 0) {
        EPRINTF("err: %d", num_nodes);
    }

    printf("\tnumber of nodes = %d\n\n", num_nodes);
}

static void
print_routing_table(axiom_dev_t *dev, int all_nodes)
{
    axiom_err_t err;
    uint8_t enabled_mask;
    int i, j;

    printf("\trouting table");
    if (all_nodes) {
        printf(" [all nodes]");
    } else {
        printf(" [only reachable nodes]");
    }

    printf("\n\t\tnode\tIF0\tIF1\tIF2\tIF3\n");

    for (i = 0; i < AXIOM_NODES_MAX; i++) {
        err = axiom_get_routing(dev, i, &enabled_mask);
        if (err) {
            EPRINTF("err: %x enabled_mask: %x", err, enabled_mask);
            break;
        }

        if (!all_nodes && (enabled_mask == 0))
            continue;

        printf("\t\t%d\t",i);

        for (j = 0; j < AXIOM_INTERFACES_MAX; j++) {
            int on = ((enabled_mask & (1 << j)) != 0);

            printf("%d\t", on);
        }

        printf("\n");
    }
    printf("\n");
}

static void
print_ni_status(axiom_dev_t *dev)
{
    uint32_t status;

    status = axiom_read_ni_status(dev);

    printf("\tstatus register = 0x%08x\n", status);

    /*TODO*/

    printf("\n");
}


static void
print_ni_control(axiom_dev_t *dev)
{
    uint32_t control;
    int loopback;

    control = axiom_read_ni_control(dev);

    printf("\tcontrol register = 0x%08x\n", control);

    loopback =  ((control & AXIOMREG_CONTROL_LOOPBACK) != 0);
    printf("\t\tLOOPBACK = %d\n", loopback);

    printf("\n");
}

int
main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    uint16_t print_bitmap = 0;

    int long_index =0;
    int opt = 0;

    static struct option long_options[] = {
        {"all", no_argument, 0, 'a'},
        {"nodeid", no_argument, 0, 'n'},
        {"ifnumber", no_argument, 0, 'i'},
        {"ifinfo", required_argument, 0, 'f'},
        {"numnodes", no_argument, 0, 'N'},
        {"routing", no_argument, 0, 'r'},
        {"routing-all", no_argument, 0, 'R'},
        {"status", no_argument, 0, 's'},
        {"control", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "anifrRsch",
                    long_options, &long_index )) != -1) {
        switch(opt) {
            case 'a':
                print_bitmap |= PRINT_ALL;
                break;

            case 'n':
                print_bitmap |= PRINT_NODEID;
                break;

            case 'i':
                print_bitmap |= PRINT_IFNUMBER;
                break;

            case 'f':
                print_bitmap |= PRINT_IFINFO;
                break;

            case 'N':
                print_bitmap |= PRINT_NUMNODES;
                break;

            case 'r':
                print_bitmap |= PRINT_ROUTING;
                break;

            case 'R':
                print_bitmap |= PRINT_ROUTING | PRINT_ROUTING_ALL;
                break;

            case 's':
                print_bitmap |= PRINT_STATUS;
                break;

            case 'c':
                print_bitmap |= PRINT_CONTROL;
                break;

            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    if (print_bitmap == 0) {
        print_bitmap = PRINT_ALL_RTSET;
    }

    printf("AXIOM NIC informations\n");
    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL)
    {
        perror("axiom_open()");
        exit(-1);
    }

    if (print_bitmap & PRINT_NODEID)
        print_nodeid(dev);

    if (print_bitmap & PRINT_IFNUMBER)
        print_ifnumber(dev);

    if (print_bitmap & PRINT_IFINFO)
        print_ifinfo_all(dev);

    if (print_bitmap & PRINT_NUMNODES)
        print_num_nodes(dev);

    if (print_bitmap & PRINT_ROUTING)
        print_routing_table(dev, (print_bitmap & PRINT_ROUTING_ALL));

    if (print_bitmap & PRINT_STATUS)
        print_ni_status(dev);

    if (print_bitmap & PRINT_CONTROL)
        print_ni_control(dev);

    axiom_close(dev);

    return 0;
}
