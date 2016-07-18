/*!
 * \file axiom-rdma.c
 *
 * \version     v0.6
 * \date        2016-07-07
 *
 * This file contains the implementation of axiom-rdma application.
 *
 * axiom-rdma allow
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

#include "axiom_nic_types.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_packets.h"
#include "dprintf.h"

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
    axiom_node_id_t remote_id;
    uint32_t payload_size;
    axiom_addr_t local_offset;
    axiom_addr_t remote_offset;
    void *rdma_zone;
    uint64_t rdma_size;
    uint8_t payload[AXIOM_RDMA_PAYLOAD_MAX_SIZE];
} axrdma_status_t;

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-rdma [arguments] -m mode [-s payload size  payload (list of bytes)]\n");
    printf("Application to use RDMA features of Axiom NIC\n");
    printf("\n\n");
    printf("Arguments:\n");
    printf("-m, --mode      r,w,d,s  r = rdma read, w = rdma write, d = local dump,");
    printf("                         s = local store\n");
    printf("-n, --node      node_id  remote node id \n");
    printf("-o, --loffset   offset   local offset in the RDMA zone [default 0]\n");
    printf("-O, --roffset   offset   remote offset in the RDMA zone [default 0]\n");
    printf("-s, --size      size     size of payload\n");
    printf("-p, --port      port     port used for the RDMA transfer\n");
    printf("-v, --verbose            verbose\n");
    printf("-h, --help               print this help\n\n");
}

void
axrdma_read(axrdma_status_t *s)
{
    axiom_err_t ret;
    axiom_rdma_payload_size_t rdma_psize =
        s->payload_size >> AXIOM_RDMA_PAYLOAD_SIZE_ORDER;

    if (s->remote_id == AXIOM_NULL_NODE) {
        EPRINTF("You must specify the remote node [-n]");
        return;
    }

    if (s->payload_size == 0 ||
            (s->payload_size > AXIOM_RDMA_PAYLOAD_MAX_SIZE)) {
        EPRINTF("You must specify the payload size (max %d) [-s]",
                AXIOM_RDMA_PAYLOAD_MAX_SIZE);
        return;
    }

    if (rdma_psize == 0) {
        EPRINTF("You must specify the payload size (multiple of 8 bytes) [-s]");
        return;
    }

    printf("[node %u] RDMA read - remote_id: %u size: %u local_offset: 0x%"
            PRIx32 " remote_offset: 0x%" PRIx32 "\n",
            s->local_id, s->remote_id,
            ((uint32_t)(rdma_psize)) << AXIOM_RDMA_PAYLOAD_SIZE_ORDER,
            s->local_offset, s->remote_offset);

    ret = axiom_rdma_read(s->dev, s->remote_id, s->port, rdma_psize,
            s->remote_offset, s->local_offset);

    if (ret != AXIOM_RET_OK) {
        EPRINTF("axiom_rdma_read() failed - ret: 0x%x", ret);
        return;
    }
}

void
axrdma_write(axrdma_status_t *s)
{
    axiom_err_t ret;
    axiom_rdma_payload_size_t rdma_psize =
        s->payload_size >> AXIOM_RDMA_PAYLOAD_SIZE_ORDER;

    if (s->remote_id == AXIOM_NULL_NODE) {
        EPRINTF("You must specify the remote node [-n]");
        return;
    }

    if (s->payload_size == 0 ||
            (s->payload_size > AXIOM_RDMA_PAYLOAD_MAX_SIZE)) {
        EPRINTF("You must specify the payload size (max %d) [-s]",
                AXIOM_RDMA_PAYLOAD_MAX_SIZE);
        return;
    }

    if (rdma_psize == 0) {
        EPRINTF("You must specify the payload size (multiple of 8 bytes) [-s]");
        return;
    }

    printf("[node %u] RDMA write - remote_id: %u size: %u local_offset: 0x%"
            PRIx32 " remote_offset: 0x%" PRIx32 "\n",
            s->local_id, s->remote_id,
            ((uint32_t)(rdma_psize)) << AXIOM_RDMA_PAYLOAD_SIZE_ORDER,
            s->local_offset, s->remote_offset);

    ret = axiom_rdma_write(s->dev, s->remote_id, s->port, rdma_psize,
            s->local_offset, s->remote_offset);

    if (ret != AXIOM_RET_OK) {
        EPRINTF("axiom_rdma_write() failed - ret: 0x%x", ret);
        return;
    }
}

void
axrdma_dump(axrdma_status_t *s)
{
    uint8_t *dump;
    int i;

    if (s->payload_size == 0) {
        EPRINTF("You must specify the dump size [-s]");
        return;
    }

    dump = s->rdma_zone + s->local_offset;

    printf("[node %u] RDMA local dump - start: %p end: %p size: %" PRIu32,
            s->local_id, dump, dump + s->payload_size - 1, s->payload_size);

    for (i = 0; i < s->payload_size; i++) {
        if (!(i % 16))
            printf("\n%08x ", s->local_offset + i);

        printf("%02x ", dump[i]);
    }

    printf("\n");
}

void
axrdma_store(axrdma_status_t *s)
{
    void *start = s->rdma_zone + s->local_offset;

    printf("[node %u] RDMA local store - start: %p end: %p size: %u\n",
            s->local_id, start, start + s->payload_size - 1, s->payload_size);

    memcpy(start, &s->payload, s->payload_size);

    if (verbose)
        axrdma_dump(s);
}

int
main(int argc, char **argv)
{
    axrdma_status_t s = {
        .dev = NULL,
        .mode = -1,
        .port = 1,
        .remote_id = AXIOM_NULL_NODE,
        .payload_size = 0,
        .local_offset = 0,
        .remote_offset = 0
    };
    char rdma_mode_char;
    int long_index =0;
    int opt = 0;
    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'},
        {"node", required_argument, 0, 'n'},
        {"loffset", required_argument, 0, 'o'},
        {"roffset", required_argument, 0, 'O'},
        {"size", required_argument, 0, 's'},
        {"port", required_argument, 0, 'p'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"m:n:o:O:s:fp:vh",
                         long_options, &long_index )) != -1) {
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
                if (sscanf(optarg, "%" SCNu8, &s.remote_id) != 1) {
                    EPRINTF("wrong remote id [8bit]");
                    usage();
                    exit(-1);
                }
                break;
            case 'o':
                if (sscanf(optarg, "%" SCNu32, &s.local_offset) != 1) {
                    EPRINTF("wrong local offset [32bit]");
                    usage();
                    exit(-1);
                }
                break;
            case 'O':
                if (sscanf(optarg, "%" SCNu32, &s.remote_offset) != 1) {
                    EPRINTF("wrong remote offset [32bit]");
                    usage();
                    exit(-1);
                }
                break;
            case 's':
                if (sscanf(optarg, "%" SCNu32, &s.payload_size) != 1) {
                    EPRINTF("wrong payload size [16bit]");
                    usage();
                    exit(-1);
                }
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
            case 'h':
            default:
                usage();
                exit(-1);
        }
    }

    /* read payload list */
    if (s.mode == LOCAL_STORE) {
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

    /* bind the current process on port */
#if 0
    err = axiom_bind(dev, port);
#endif

    /* map rdma zone */
    s.rdma_zone = axiom_rdma_mmap(s.dev, &s.rdma_size);
    if (!s.rdma_zone) {
        EPRINTF("rdma map failed");
        goto err;
    }

    IPRINTF(verbose, "rdma_mmap - addr: %p size: %" PRIu64,
            s.rdma_zone, s.rdma_size);

    if (s.payload_size + s.local_offset > s.rdma_size) {
        EPRINTF("Out of RDMA zone - rdma_size: %" PRIu64, s.rdma_size);
        goto err;
    }

    IPRINTF(verbose, "start");

    /* check parameter */
    switch (s.mode) {
        case RDMA_READ:
            axrdma_read(&s);
            break;
        case RDMA_WRITE:
            axrdma_write(&s);
            break;
        case LOCAL_DUMP:
            axrdma_dump(&s);
            break;
        case LOCAL_STORE:
            axrdma_store(&s);
            break;
        default:
            EPRINTF("You must specify the mode [-m]");
            usage();
            exit(-1);
    }

    IPRINTF(verbose, "end");

err:
    axiom_close(s.dev);

    return 0;
}
