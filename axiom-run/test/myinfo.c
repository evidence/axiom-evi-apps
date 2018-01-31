/*!
 * \file myinfo.c
 *
 * \version     v1.0
 *
 * A simple program to test axiom-run stdio redirection.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#define PSLEEP() {if (pdelay>0.0) { if (myrand==0.0) usleep(pdelay*1e6); else usleep((pdelay+myrand*((double)rand()/RAND_MAX-0.5))*1e6);}}
#define SLEEP() {if (delay>0.0) {if (myrand==0.0) usleep(delay*1e6); else usleep((delay+myrand*((double)rand()/RAND_MAX-0.5))*1e6);} if (autoflush) fflush(fout); }
#define ID() { if (id>=0) fprintf(fout, "{%d}",id); }

void mysigquit(int dummy) {
    // NB: not fout but stderr!!!
    fprintf(stderr, "SIGQUIT called\n");
    exit(-1);
}

/* 
 * Emit running enviroment to FILE*
 * to test output redirection
 *
 */

static void show(FILE *fout, double delay, double pdelay, double myrand, int autoflush, int input, int startrand, int id, int argc, char **argv, char **env) {
    int i;

    if (myrand != 0.0) {
        int n, h;
        h = open("/dev/urandom", O_RDONLY);
        n = 17;
        if (h > 0) {
            read(h, &n, sizeof (n));
            close(h);
        }
        srand(n);
    }

    if (startrand) {
        usleep((double) rand() / RAND_MAX * 1000000);
    }

    ID();
    fprintf(fout, "START\n");
    PSLEEP();
    SLEEP();
    ID();
    fprintf(fout, "ppid:     ");
    PSLEEP();
    fprintf(fout, "%d\n", getppid());
    PSLEEP();
    SLEEP();
    ID();
    fprintf(fout, "pid:      ");
    PSLEEP();
    fprintf(fout, "%d\n", getpid());
    PSLEEP();
    SLEEP();
    ID();
    fprintf(fout, "argc:     ");
    PSLEEP();
    fprintf(fout, "%d\n", argc);
    PSLEEP();
    SLEEP();
    for (i = 0; i < argc; i++) {
        ID();
        fprintf(fout, "argv[%2d]: ", i);
        PSLEEP();
        fprintf(fout, "%s\n", argv[i]);
        PSLEEP();
        SLEEP();
    }
    while (*env != NULL) {
        ID();
        fprintf(fout, "env:      ");
        PSLEEP();
        fprintf(fout, "%s\n", *env++);
        PSLEEP();
        SLEEP();
    }

    if (input) {
        static char buf[512];
        char *s;
        ID();
        fprintf(fout, "waiting input...\n");

        s = fgets(buf, sizeof (buf), stdin);
        if (s == NULL) {
            ID();
            fprintf(fout, "error!\n");
            PSLEEP();
        } else {
            char *p = s;
            while (*p != '\0') {
                if (!isprint(*p)) *p = '@';
                p++;
            }
            ID();
            fprintf(fout, "in:       ");
            PSLEEP();
            fprintf(fout, "%s\n", s);
            PSLEEP();
        }
        SLEEP();
    }

    ID();
    fprintf(fout, "END\n");
    PSLEEP();
    SLEEP();
    fflush(fout);
}

extern char **environ;

static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"flush", no_argument, 0, 'f'},
    {"delay", required_argument, 0, 's'},
    {"output", required_argument, 0, 'o'},
    {"mode", required_argument, 0, 'm'},
    {"pdelay", required_argument, 0, 'p'},
    {"rand", required_argument, 0, 'r'},
    {"in", no_argument, 0, 'i'},
    {"srand", no_argument, 0, 'd'},
    {"error", no_argument, 0, 'e'},
    {0, 0, 0, 0}
};

int main(int argc, char**argv) {
    FILE *fout = NULL;
    FILE *fin = NULL;
    double delay = 0.0;
    double pdelay = 0.0;
    double myrand = 0.0;
    int autoflush = 0;
    int mode = -1;
    int mymode = 0;
    int id = -1;
    int error = 0;
    int input = 0;
    int startrandomdelay = 0;
    int opt;
    int long_index;

    signal(SIGQUIT, mysigquit);

    opterr = 0;
    while ((opt = getopt_long(argc, argv, "hs:o:fp:m:r:eid", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'o':
                fout = fopen(optarg, "w+");
                break;
            case 's':
                delay = strtod(optarg, NULL);
                break;
            case 'p':
                pdelay = strtod(optarg, NULL);
                break;
            case 'r':
                myrand = strtod(optarg, NULL);
                break;
            case 'm':
                mode = atoi(optarg);
                switch (mode) {
                    case 0: mymode = _IONBF;
                        break;
                    case 1: mymode = _IOLBF;
                        break;
                    case 2: mymode = _IOFBF;
                        break;
                    default: mymode = _IOLBF;
                        break;
                }
                break;
            case 'f':
                autoflush = 1;
                break;
            case 'd':
                startrandomdelay = 1;
                break;
            case 'e':
                error = 1;
                break;
            case 'i':
                input = 1;
                break;
            case 'h':
                fprintf(stderr, "usage: myinfo [-h|--help|-o OUTFILE|--output OUTFILE|-s DELAY|--delay DELAY|-p DELAY|--pdelay PDELAY|-f|--flush|-m BUFMODE|--mode BUFMODE|-r RAND|--randomize RAND|-e|--error|-i|--in|-r|--randexit|-d|--srand]\n");
                fprintf(stderr, "where\n");
                fprintf(stderr, "  -h          is this help screen\n");
                fprintf(stderr, "  -o OUTFILE  is the output file [default: stdout]\n");
                fprintf(stderr, "  -s DELAY    is the delay (in seconds, float number) between\n");
                fprintf(stderr, "              writing a fully line to output [deault: 0]\n");
                fprintf(stderr, "  -p PDELAY   is the delay (in seconds, float number) between\n");
                fprintf(stderr, "              writing to output [deault: 0]\n");
                fprintf(stderr, "  -f          enable flush after every printf\n");
                fprintf(stderr, "  -m BUFMODE  output buffering mode [default: unix default]\n");
                fprintf(stderr, "              0 = unbuffered\n");
                fprintf(stderr, "              1 = line buffered\n");
                fprintf(stderr, "              2 = fully buffered\n");
                fprintf(stderr, "  -r RAND     randomize delays with a uniform distribution of +/- RAND/2 seconds\n");
                fprintf(stderr, "  -d          start random delay\n");
                fprintf(stderr, "  -e          output on stderr (unless -o is used)\n");
                fprintf(stderr, "  -i          read something from stdin (and write it on stdout/stderr/file)\n");
                fprintf(stderr, "\n\nid.txt, if present, must containt the application_id_number\n");
                exit(0);
            case '?':
                break;
        }

    }

    if (mode != -1) {
        if (setvbuf(fout == NULL ? (error ? stderr : stdout) : fout, NULL, mymode, BUFSIZ) != 0) {
            perror("setvbuf() error!");
        }
    }

    fin = fopen("id.txt", "r");
    if (fin != NULL) {
        fscanf(fin, "%d", &id);
        fclose(fin);
    }

    show(fout == NULL ? (error ? stderr : stdout) : fout, delay, pdelay, myrand, autoflush, input, startrandomdelay, id, argc, argv, environ);
    
    return EXIT_SUCCESS;
}
