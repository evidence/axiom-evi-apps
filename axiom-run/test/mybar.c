
/*
 * A simple test program to test axiom-run barriers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "axiom_run_api.h"

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"barrier", required_argument, 0, 'b'},
    {"sleep", required_argument, 0, 's'},
    {0, 0, 0, 0}
};

static int id = -1;

void mysigquit(int dummy) {
    fprintf(stderr, "{%d} SIGQUIT called\n", id);
    exit(EXIT_FAILURE);
}

int main(int argc, char**argv) {
    unsigned barrier = AXRUN_MAX_BARRIER_ID;
    useconds_t delay = 0;
    int opt, long_index, res;
    double n;
    
    signal(SIGQUIT, mysigquit);

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hb:s:", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'b':
                barrier = atoi(optarg);
                break;
            case 's':
                n = strtod(optarg, NULL);
                delay = (useconds_t) (n * 1e6);
                break;
            case 'h':
            case '?':
                fprintf(stderr, "usage: mybar [ARGS]*\n");
                fprintf(stderr, "where ARGS\n");
                fprintf(stderr, "  -h                is this help screen\n");
                fprintf(stderr, "  -b|--barrier ID   barrier id\n");
                fprintf(stderr, "  -s|--sleep SEC    initial delay in seconds\n");
                fprintf(stderr, "\n\nid.txt, if present, must containt the application_id_number\n");
                exit(0);
        }
    }

    {
        //
        // if the file id.txt exist use it prior to print messages to stdout
        //
        FILE *fin = fopen("id.txt", "r");
        if (fin != NULL) {
            fscanf(fin, "%d", &id);
            fclose(fin);
        }
    }

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    if (delay > 0) {
        fprintf(stdout, "{%d} initial delay...\n", id);
        usleep(delay);
    }
    fprintf(stdout, "{%d} sync on barrier %d...\n", id, barrier);
    for (;;) {
        res = axrun_sync(barrier);
        if (res == 0) break;
        fprintf(stderr, "{%d} error on axrun_sync() errno=%d '%s'\n", id, errno, strerror(errno));
        if (errno != EAGAIN) break;
    }
    fprintf(stdout, "{%d} sync done\n", id);

    return EXIT_SUCCESS;
}
