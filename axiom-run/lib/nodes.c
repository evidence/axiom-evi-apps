/*!
 * \file nodes.c
 *
 * \version     v1.1
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "axiom_run_api.h"
#include "dprintf.h"

#define AXIOM_ENV_RUN_NODES     "AXIOM_NODES"
#define AXIOM_ENV_RUN_NUMNODES  "AXIOM_RUN"

uint64_t axrun_get_nodes()
{
    char *env_s;

    env_s = getenv(AXIOM_ENV_RUN_NODES);
    if (env_s == NULL) {
        EPRINTF("env %s not defined", AXIOM_ENV_RUN_NODES);
        return 0;
    }

    return (uint64_t)strtoull(env_s, NULL, 0);
}

int axrun_get_num_nodes()
{
    char *env_s;

    env_s = getenv(AXIOM_ENV_RUN_NUMNODES);
    if (env_s == NULL) {
        EPRINTF("env %s not defined", AXIOM_ENV_RUN_NUMNODES);
        return -1;
    }

    return (int)strtoul(env_s, NULL, 0);
}
