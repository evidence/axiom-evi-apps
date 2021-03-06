/*!
 * \file axiom-rdma.c
 *
 * \version     v1.2
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
    unsigned long local_offset;
    unsigned long remote_offset;
    void *rdma_zone;
    uint64_t rdma_size;
    uint8_t payload[AXIOM_RDMA_PAYLOAD_MAX_SIZE];
} axrdma_status_t;

int verbose = 0;

static void
usage(void)
{
    printf("usage: axiom-rdma-dbg [arguments] -m mode [-s payload size  payload (list of bytes)]\n");
    printf("Application to debug RDMA features of Axiom NIC\n");
    printf("Version: %s\n", AXIOM_API_VERSION_STR);
    printf("\n\n");
    printf("Arguments:\n");
    printf("-m, --mode      r,w,d,s        r = rdma read, w = rdma write, d = local dump,\n");
    printf("                               s = local store\n");
    printf("-n, --node      node_id        remote node id \n");
    printf("-o, --loffset   off[B|K|M|G]   local offset in the RDMA zone [default 0]\n");
    printf("-O, --roffset   off[B|K|M|G]   remote offset in the RDMA zone [default 0]\n");
    printf("-s, --size      size[B|K|M|G]  size of payload in byte\n");
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

void
axrdma_read(axrdma_status_t *s)
{
    axiom_err_t ret;

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

    printf("[node %u] RDMA read - remote_id: %u size: %u local_offset: 0x%lx remote_offset: 0x%lx\n",
            s->local_id, s->remote_id,
            s->payload_size,
            s->local_offset, s->remote_offset);

    ret = axiom_rdma_read_sync(s->dev, s->remote_id, s->payload_size,
            (void *)s->remote_offset, (void *)s->local_offset, NULL);

    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_rdma_read() failed - ret: 0x%x", ret);
        if (ret == AXIOM_RET_NOTREACH) {
            printf("Destination node id not reachable [%u]\n", s->remote_id);
        }
    }
}

void
axrdma_write(axrdma_status_t *s)
{
    axiom_err_t ret;

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

    printf("[node %u] RDMA write - remote_id: %u size: %u local_offset: 0x%lx remote_offset: 0x%lx\n",
            s->local_id, s->remote_id,
            s->payload_size,
            s->local_offset, s->remote_offset);

    ret = axiom_rdma_write_sync(s->dev, s->remote_id, s->payload_size,
            (void *)s->local_offset, (void *)s->remote_offset, NULL);

    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_rdma_write() failed - ret: 0x%x", ret);
        if (ret == AXIOM_RET_NOTREACH) {
            printf("Destination node id not reachable [%u]\n", s->remote_id);
        }
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
            printf("\n%08lx ", s->local_offset + i);

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
        {"version", no_argument, 0, 'V'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };


    while ((opt = getopt_long(argc, argv,"m:n:o:O:s:fp:vVh",
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
                if (sscanf(optarg, "%" SCNu8, &s.remote_id) != 1) {
                    EPRINTF("wrong remote id [8bit]");
                    usage();
                    exit(-1);
                }
                break;
            case 'o':
                if (sscanf(optarg, "%lu%c", &s.local_offset,
                            &char_scale) == 0) {
                    EPRINTF("wrong local offset [32bit]");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
                break;
            case 'O':
                if (sscanf(optarg, "%lu%c", &s.remote_offset,
                            &char_scale) == 0) {
                    EPRINTF("wrong remote offset [32bit]");
                    usage();
                    exit(-1);
                }
                data_scale = get_scale(char_scale);
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
            goto err;
    }

    IPRINTF(verbose, "end");

err:
    axiom_close(s.dev);

    return 0;
}
