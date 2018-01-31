/*!
 * \file axiom-rdma.c
 *
 * \version     v1.0
 * \date        2016-07-07
 *
 * This file contains the implementation of axiom-rdma application.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "axiom_allocator.h"
#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "axiom_run_api.h"
#include "dprintf.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define AXIOM_RDMA_SYNC_ID              12

typedef enum {
    RDMA_READ = 'r',
    RDMA_WRITE = 'w',
    LOCAL_DUMP = 'd',
    LOCAL_STORE = 's'
} axrdma_mode_t;

typedef struct axrdma_status {
    axiom_dev_t *dev;
    axrdma_mode_t mode;
    axiom_port_t port;
    axiom_node_id_t local_id;
    axiom_node_id_t master_id;
    axiom_node_id_t slave_id;
    uint32_t payload_size;
    axiom_addr_t master_offset;
    axiom_addr_t slave_offset;
    uint8_t *rdma_start;
    size_t rdma_size;
    uint8_t payload[AXIOM_RDMA_PAYLOAD_MAX_SIZE];
} axrdma_status_t;

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-rdma [arguments] -m mode [-s payload size  payload (list of bytes)]\n");
    printf("Application to use RDMA features of Axiom NIC\n");
    printf("Must be run with axiom-run (eg. axiom-run -P all axiom-rdma -m r -s 1k 1\n");
    printf("Version: %s\n", AXIOM_API_VERSION_STR);
    printf("\n\n");
    printf("Arguments:\n");
    printf("-m, --mode      r,w,d,s        r = rdma read, w = rdma write\n");
    printf("-n, --sid       node_id        slave node id \n");
    printf("-N, --mid       node_id        master node id \n");
    printf("-o, --soff      off[B|K|M|G]   slave offset in the RDMA zone [default 0]\n");
    printf("-O, --moff      off[B|K|M|G]   master offset in the RDMA zone [default 0]\n");
    printf("-s, --size      size[B|K|M|G]  size of region to transfer in byte\n");
    printf("                               The suffix specifies the size unit\n");
    printf("-p, --port      port           port used for the RDMA transfer\n");
    printf("-v, --verbose                  verbose\n");
    printf("-V, --version                  print version\n");
    printf("-h, --help                     print this help\n\n");
}

static int
get_scale(char char_scale) {
    uint8_t scale;

    switch (char_scale) {
        case 'b': case 'B':
            scale = 0;
            break;

        case 'k': case 'K':
            scale = 10;
            break;

        case 'm': case 'M':
            scale = 20;
            break;

        case 'G': case 'g':
            scale = 30;
            break;

        default:
            scale = 0;
    }

    return scale;
}

int
axrdma_read(axrdma_status_t *s)
{
    axiom_err_t ret;

    if (s->slave_id == AXIOM_NULL_NODE) {
        EPRINTF("You must specify the remote node [-n]");
        return -1;
    }

    if (s->payload_size == 0 ||
            (s->payload_size > AXIOM_RDMA_PAYLOAD_MAX_SIZE)) {
        EPRINTF("You must specify the payload size (max %d) [-s]",
                AXIOM_RDMA_PAYLOAD_MAX_SIZE);
        return -1;
    }

    printf("[node %u] RDMA read - slave_id: %u size: %" PRIu32" start: %p "
            "master_offset: 0x%" PRIx32 " slave_offset: 0x%" PRIx32 "\n",
            s->local_id, s->slave_id, s->payload_size, s->rdma_start,
            s->master_offset, s->slave_offset);

    ret = axiom_rdma_read_sync(s->dev, s->slave_id, s->payload_size,
            s->rdma_start + s->slave_offset, s->rdma_start + s->master_offset,
            NULL);

    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_rdma_read() failed - ret: 0x%x", ret);
        if (ret == AXIOM_RET_NOTREACH) {
            printf("Destination node id not reachable [%u]\n", s->slave_id);
        }
        return -1;
    }

    return 0;
}

int
axrdma_write(axrdma_status_t *s)
{
    axiom_err_t ret;

    if (s->slave_id == AXIOM_NULL_NODE) {
        EPRINTF("You must specify the remote node [-n]");
        return -1;
    }

    if (s->payload_size == 0 ||
            (s->payload_size > AXIOM_RDMA_PAYLOAD_MAX_SIZE)) {
        EPRINTF("You must specify the payload size (max %d) [-s]",
                AXIOM_RDMA_PAYLOAD_MAX_SIZE);
        return -1;
    }

    printf("[node %u] RDMA write - slave_id: %u size: %" PRIu32 " start: %p "
            "master_offset: 0x%" PRIx32 " slave_offset: 0x%" PRIx32 "\n",
            s->local_id, s->slave_id, s->payload_size, s->rdma_start,
            s->master_offset, s->slave_offset);

    ret = axiom_rdma_write_sync(s->dev, s->slave_id, s->payload_size,
            s->rdma_start + s->master_offset, s->rdma_start + s->slave_offset,
            NULL);

    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_rdma_write() failed - ret: 0x%x", ret);
        if (ret == AXIOM_RET_NOTREACH) {
            printf("Destination node id not reachable [%u]\n", s->slave_id);
        }
        return -1;
    }

    return 0;
}

int
axrdma_dump(axrdma_status_t *s)
{
    uint32_t offset;
    uint8_t *dump;
    int i;

    if (s->payload_size == 0) {
        EPRINTF("You must specify the dump size [-s]");
        return -1;
    }

    if (s->local_id == s->master_id) {
        offset = s->master_offset;
    } else {
        offset = s->slave_offset;
    }

    dump = s->rdma_start + offset;

    printf("[node %u] RDMA local dump - start: %p end: %p size: %" PRIu32,
            s->local_id, dump, dump + s->payload_size - 1, s->payload_size);

    for (i = 0; i < s->payload_size; i++) {
        if (!(i % 16))
            printf("\n%08x ", offset + i);

        printf("%02x ", dump[i]);
    }

    printf("\n");
    fflush(stdout);

    return 0;
}

int
axrdma_store(axrdma_status_t *s)
{
    uint32_t offset;
    void *start;

    if (s->local_id == s->master_id) {
        offset = s->master_offset;
    } else {
        offset = s->slave_offset;
    }

    start = s->rdma_start + offset;

    printf("[node %u] RDMA local store - start: %p end: %p size: %u\n",
            s->local_id, start, start + s->payload_size - 1, s->payload_size);

    memcpy(start, &s->payload, s->payload_size);

    return 0;
}

int
main(int argc, char **argv)
{
    axrdma_status_t s = {
        .dev = NULL,
        .mode = -1,
        .port = 1,
        .master_id = AXIOM_NULL_NODE,
        .slave_id = AXIOM_NULL_NODE,
        .payload_size = 0,
        .master_offset = 0,
        .slave_offset = 0
    };
    char rdma_mode_char;
    int long_index = 0, opt = 0, ret;
    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'},
        {"sid", required_argument, 0, 'n'},
        {"mid", required_argument, 0, 'N'},
        {"soff", required_argument, 0, 'o'},
        {"moff", required_argument, 0, 'O'},
        {"size", required_argument, 0, 's'},
        {"port", required_argument, 0, 'p'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int num_nodes, node_l, node_t;
    uint64_t mask_nodes;


    while ((opt = getopt_long(argc, argv,"m:n:N:o:O:s:fp:vVh",
                         long_options, &long_index )) != -1) {
        char char_scale = 'b';
        int data_scale = 0;

        switch (opt) {
            case 'm':
                if (sscanf(optarg, "%c", &rdma_mode_char) != 1) {
                    EPRINTF("wrong mode parameter");
                    usage();
                    exit(-1);
                }
                s.mode = rdma_mode_char;
                break;
            case 'n':
                if (sscanf(optarg, "%" SCNu8, &s.slave_id) != 1) {
                    EPRINTF("wrong slave id [8bit]");
                    usage();
                    exit(-1);
                }
                break;
            case 'N':
                if (sscanf(optarg, "%" SCNu8, &s.master_id) != 1) {
                    EPRINTF("wrong master id [8bit]");
                    usage();
                    exit(-1);
                }
                break;
            case 'o':
                if (sscanf(optarg, "%" SCNu32 "%c", &s.slave_offset,
                            &char_scale) == 0) {
                    EPRINTF("wrong slave offset [32bit]");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                s.slave_offset = s.slave_offset << data_scale;
                break;
            case 'O':
                if (sscanf(optarg, "%" SCNu32 "%c", &s.master_offset,
                            &char_scale) == 0) {
                    EPRINTF("wrong master offset [32bit]");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                s.master_offset = s.master_offset << data_scale;
                break;
            case 's':
                if (sscanf(optarg, "%" SCNu32 "%c", &s.payload_size,
                            &char_scale) == 0) {
                    EPRINTF("wrong payload size [16bit]");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                s.payload_size = s.payload_size << data_scale;
                break;
            case 'p':
                if (sscanf(optarg, "%" SCNu8, &s.port) != 1) {
                    EPRINTF("wrong port");
                    usage();
                    exit(-1);
                }
                /* check port parameter */
                if (((s.port < 0) || (s.port > 7))) {
                    EPRINTF("port not allowed [%u]; [0 <= port <= 7]\n", s.port);
                    exit(-1);
                }
                break;
            case 'v':
                verbose = 1;
                break;
            case 'V':
                printf("Version: %s\n", AXIOM_API_VERSION_STR);
                exit(0);
            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    /* read payload list */
    if (s.mode == LOCAL_STORE || s.mode == RDMA_READ || s.mode == RDMA_WRITE) {
        uint32_t payload_read = 0, orig_psize, i;

        if (s.payload_size == 0 || (s.payload_size > sizeof(s.payload))) {
            EPRINTF("You must specify the payload size (max %lu) [-s]",
                    sizeof(s.payload));
            exit(-1);
        }

        for (i = 0; (i < (argc - optind)) && (i < sizeof(s.payload)); i++) {
            if (sscanf(argv[i + optind], "%" SCNu8, &s.payload[i]) != 1) {
                EPRINTF("wrong payload byte [index = %d]", i);
                exit(-1);
            }
            payload_read++;
        }

        if (payload_read == 0) {
            EPRINTF("You must specify at least one byte of payload");
            exit(-1);
        }

        orig_psize = payload_read;

        while (payload_read + orig_psize <= s.payload_size) {
            memcpy(&s.payload[payload_read], &s.payload[0], orig_psize);
            payload_read += orig_psize;
        }

        if ((s.payload_size - payload_read) > 0) {
            memcpy(&s.payload[payload_read], &s.payload[0],
                    s.payload_size - payload_read);
        }
    }

    /* open the axiom device */
    s.dev = axiom_open(NULL);
    if (s.dev == NULL) {
        perror("axiom_open()");
        exit(-1);
    }

    s.local_id = axiom_get_node_id(s.dev);

    num_nodes = axrun_get_num_nodes();
    mask_nodes = axrun_get_nodes();

    if (num_nodes != 2) {
        EPRINTF("You must run this application through axiom-run on 2 nodes - "
                "nodes: %d", num_nodes);
        exit(-1);
    }

    node_t = __builtin_ctzll(mask_nodes);
    node_l = (sizeof(mask_nodes) * 8) - 1 - __builtin_clzll(mask_nodes);

    if (s.master_id == AXIOM_NULL_NODE) {
        if (s.slave_id == node_t)
            s.master_id = node_l;
        else
            s.master_id = node_t;
    }

    if (s.slave_id == AXIOM_NULL_NODE) {
        if (s.master_id == node_l)
            s.slave_id = node_t;
        else
            s.slave_id = node_l;
    }

    if (mask_nodes != ((1ll << s.master_id) | (1ll << s.slave_id))) {
        EPRINTF("You must run this application through axiom-run on the same "
                "nodes specified in the arguments - "
                "axiom-run nodes: %d, %d - app nodes: %d, %d",
                node_t, node_l, s.master_id, s.slave_id);
        exit(-1);
    }

    IPRINTF(1, "nodes: 0x%lx num_nodes: %d", mask_nodes, num_nodes);

    if (s.mode == RDMA_READ || s.mode == RDMA_WRITE) {
        size_t shared_size = 0;
        size_t max_size = s.payload_size + MAX(s.master_offset, s.slave_offset);

        s.rdma_size = max_size;
        ret = axiom_allocator_init(&s.rdma_size, &shared_size, AXAL_SW);
        if (ret) {
            EPRINTF("axiom_allocator_init error %d", ret);
            goto err;
        }
        IPRINTF(verbose, "allocator_init - size: %zu", s.rdma_size);

        s.rdma_start = axiom_private_malloc(max_size);
        if (!s.rdma_start) {
            EPRINTF("axiom_private_malloc error - addr: %p size: %zu",
                    s.rdma_start, max_size);
            goto err;
        }

        /* wait all nodes after the private malloc */
        ret = axrun_sync(AXIOM_RDMA_SYNC_ID, verbose);
        if (ret < 0) {
            EPRINTF("axrun_sync error %d", ret);
            goto err;
        }

        IPRINTF(verbose, "RDMA region allocated - addr: %p size: %zu",
                s.rdma_start, max_size);

    } else {
        /* map rdma zone */
        s.rdma_start = axiom_rdma_mmap(s.dev, &s.rdma_size);
        if (!s.rdma_start) {
            EPRINTF("rdma map failed");
            goto err;
        }

        IPRINTF(verbose, "rdma_mmap - addr: %p size: %" PRIu64,
                s.rdma_start, s.rdma_size);

        if (s.payload_size + s.master_offset > s.rdma_size) {
            EPRINTF("Out of RDMA zone - rdma_size: %" PRIu64, s.rdma_size);
            goto err;
        }
        s.slave_id = -1;
    }


    IPRINTF(verbose, "start");

    switch (s.mode) {
        case RDMA_READ:
            if (s.local_id == s.slave_id) {
                axrdma_store(&s);
            }

            /* wait all nodes before to do remote read */
            ret = axrun_sync(AXIOM_RDMA_SYNC_ID, verbose);
            if (ret < 0) {
                EPRINTF("axrun_sync error %d", ret);
                break;
            }

            if (s.local_id == s.master_id) {
                ret = axrdma_read(&s);
                if (ret)
                    break;
            }

            if (verbose)
                axrdma_dump(&s);
            break;
        case RDMA_WRITE: {
            int write_ret = 0;

            if (s.local_id == s.master_id) {
                axrdma_store(&s);
                write_ret = axrdma_write(&s);
            }

            /* wait all nodes before to dump memory */
            ret = axrun_sync(AXIOM_RDMA_SYNC_ID, verbose);
            if (ret < 0 || write_ret) {
                break;
            }

            if (verbose)
                axrdma_dump(&s);
            break;
        }
        case LOCAL_DUMP:
            axrdma_dump(&s);
            break;
        case LOCAL_STORE:
            axrdma_store(&s);
            if (verbose)
                axrdma_dump(&s);
            break;
        default:
            EPRINTF("You must specify the mode [-m]");
            usage();
            goto err;
    }

    IPRINTF(verbose, "end");

err:
    axiom_close(s.dev);

    return 0;
}
