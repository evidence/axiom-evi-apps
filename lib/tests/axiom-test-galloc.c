/*!
 * \file axiom-test-galloc.c
 *
 * \version     v0.8
 * \date        2016-09-27
 *
 * This file contains the implementation of tests for tha axiom global allocator.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "dprintf.h"
#include "axiom_nic_api_user.h"
#include "axiom_global_allocator.h"

int verbose = 1;

int main(int argc, char **argv)
{
    axiom_dev_t *dev = NULL;
    axiom_node_id_t node_id;
    axiom_err_t ret;
    uint64_t private_size, private_start, shared_size, shared_start;

    /* open the axiom device */
    dev = axiom_open(NULL);
    if (dev == NULL) {
        EPRINTF("axiom_open() error %p", dev);
        exit(-1);
    }

    node_id = axiom_get_node_id(dev);

    if (node_id == 0) {
        sleep(2);
    }

    printf ("I'm node %d\n", node_id);
    fflush(stdout);

    private_size = (1 << 20);
    shared_size = (1 << 22);

    IPRINTF(verbose, "private_size: %" PRIu64 " shared_size: %" PRIu64,
            private_size, shared_size);

    ret = axiom_galloc_init(0, node_id, &private_size, &shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_galloc_init() error %d", ret);
    }

    IPRINTF(verbose, "private_size: %" PRIu64 " shared_size: %" PRIu64,
            private_size, shared_size);

    ret = axiom_galloc_get_prregion(&private_start, &private_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_galloc_get_prregion() error %d", ret);
    }

    IPRINTF(verbose, "private_start: %" PRIu64 " private_size: %" PRIu64,
            private_start, private_size);


    shared_size = (1 << 20);

    ret = axiom_galloc_get_shregion(&shared_start, &shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_galloc_get_shregion() error %d", ret);
    }

    IPRINTF(verbose, "shared_start: %" PRIu64 " shared_size: %" PRIu64,
            shared_start, shared_size);

    shared_size = (1 << 26);

    ret = axiom_galloc_get_shregion(&shared_start, &shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        EPRINTF("axiom_galloc_get_shregion() error %d", ret);
    }

    IPRINTF(verbose, "shared_start: %" PRIu64 " shared_size: %" PRIu64,
            shared_start, shared_size);

    axiom_close(dev);
    return 0;
}
