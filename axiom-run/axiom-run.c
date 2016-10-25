/**
 * The axiom-run application.
 * 
 * @file axiom-run.c
 * @version v0.7
 */

#include <sys/eventfd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include "axiom-run.h"

/** Table to convert command code to command name. */
char *cmd_to_name[] = {"CMD_EXIT", "CMD_KILL", "CMD_SEND_TO_STDOUT", "CMD_SEND_TO_STDERR", "CMD_RECV_FROM_STDIN", "CMD_BARRIER", "CMD_RPC"};
char *rpcfunc_to_name[] = {"RPC_PING"};

/*
static char *debug_dump(char *ptr, int sz) {
    // :-(
    static char buf2[16];
    debug_dump_free();
    debug_buf = malloc(sz * 10 + 3);
    if (debug_buf == NULL) return "";
 *debug_buf = '\0';
    while (sz-- > 0) {
        if (isprint(*ptr))
            sprintf(buf2, "%c(0x%02x)", *ptr, *ptr);
        else
            sprintf(buf2, " (0x%02x)", *ptr);
        if (sz != 0) strcat(debug_buf, " ");
        strcat(debug_buf, buf2);
        ptr++;
    }
    return debug_buf;
}
 */

int master_node = -1;
int master_port = -1;
int slave_port = -1;

/**
 * Emit program usage on stderr.
 * @param msg printf style message (can be null)
 * @param ... parameters for msg
 */
static void _usage(char *msg, ...) {
    if (msg != NULL) {
        va_list list;
        va_start(list, msg);
        vfprintf(stderr, msg, list);
        va_end(list);
        fputc('\n', stderr);
    }
    fprintf(stderr, "usage: axiom-run [ARG]* APPLICATION [APP_ARG]*\n");
    fprintf(stderr, "Spawn application on multiple nodes\n");
    fprintf(stderr, "ARGs are:\n");
    fprintf(stderr, "-s, --slave\n");
    fprintf(stderr, "    slave mode\n");
    fprintf(stderr, "-p, --port NUM\n");
    fprintf(stderr, "    use axiom raw port NUM for comunication\n");
    fprintf(stderr, "    [default master: %u slave: %u]\n", MY_DEFAULT_MASTER_PORT, MY_DEFAULT_SLAVE_PORT);
    fprintf(stderr, "-m, --master [NOD|NOD,POR]\n");
    fprintf(stderr, "    the master axiom-run is on node NOD on port POR\n");
    fprintf(stderr, "    [default: 0,my_port-1]\n");
    fprintf(stderr, "-n, --nodes NODELIST\n");
    fprintf(stderr, "    nodes where spawn application \n");
    fprintf(stderr, "    [default: all nodes] ex: 0-3,7\n");
    fprintf(stderr, "-g, --gdb NODELIST:PORT\n");
    fprintf(stderr, "    run application using gdb server on port PORT on selected nodes\n");
    fprintf(stderr, "    [default: no run gdb server]\n");
    fprintf(stderr, "-u, --env REGEXP\n");
    fprintf(stderr, "    environmet to run application\n");
    fprintf(stderr, "    [default: PATH|SHELL|AXIOM_.*]\n");
    fprintf(stderr, "-r, --redirect\n");
    fprintf(stderr, "    redirect service\n");
    fprintf(stderr, "-i, --ident [MODE]\n");
    fprintf(stderr, "    the redirect service emit a node identification [default: 0]\n");
    fprintf(stderr, "    MODE 0   {NODE}\n");
    fprintf(stderr, "         1   NODE:\n");
    fprintf(stderr, "-e, --exit\n");
    fprintf(stderr, "    exit service\n");
    fprintf(stderr, "-b, --barrier\n");
    fprintf(stderr, "    barrier service\n");
    fprintf(stderr, "-c, --rpc\n");
    fprintf(stderr, "    rpc service\n");
    fprintf(stderr, "-a, --allocator\n");
    fprintf(stderr, "    allocator service: handle axiom allocator and assign an unique application ID exported in the env of all process (AXIOM_ALLOC_APPID)\n");
    fprintf(stderr, "-h, --help\n");
    fprintf(stderr, "    print this help\n");
    fprintf(stderr, "note:\n");
    fprintf(stderr, "-m is used to inform an axiom-run slave where is the master so is useless if no -s option is used\n");
}

/**
 * _usage with no message.
 */
#define usage() _usage(NULL)

/**
 * Long options for command line.
 */
static struct option long_options[] = {
    {"redirect", no_argument, 0, 'r'},
    {"ident", optional_argument, 0, 'i'},
    {"exit", no_argument, 0, 'e'},
    {"barrier", no_argument, 0, 'b'},
    {"rpc", no_argument, 0, 'c'},
    {"slave", no_argument, 0, 's'},
    {"master", required_argument, 0, 'm'},
    {"nodes", required_argument, 0, 'n'},
    {"port", required_argument, 0, 'p'},
    {"env", required_argument, 0, 'u'},
    {"gdb", required_argument, 0, 'g'},
    {"appid", no_argument, 0, 'a'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};


/* see axiom-run.h */
void block_all_signals(sigset_t *oldset) {
    sigset_t set;
    int res;
    res = sigfillset(&set);
    if (res != 0) {
        elogmsg("sigillset()");
        exit(EXIT_FAILURE);
    }
    pthread_sigmask(SIG_SETMASK, &set, oldset);
    if (res != 0) {
        elogmsg("pthread_sigmask()");
        exit(EXIT_FAILURE);
    }
}

/* see axiom-run.h */
void restore_signals_and_set_quit_handler(sigset_t *oldset, void (*myhandler)(int)) {
    struct sigaction act;
    int res;

    // handler on SIGINT, SIGQUIT and SIGTERM

    act.sa_handler = myhandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    res = sigaction(SIGINT, &act, NULL);
    if (res != 0) {
        elogmsg("sigaction()");
        exit(EXIT_FAILURE);
    }
    res = sigaction(SIGQUIT, &act, NULL);
    if (res != 0) {
        elogmsg("sigaction()");
        exit(EXIT_FAILURE);
    }
    res = sigaction(SIGTERM, &act, NULL);
    if (res != 0) {
        elogmsg("sigaction()");
        exit(EXIT_FAILURE);
    }

    // handler for SIGCHLD
    
    act.sa_handler = SIG_DFL;
    sigemptyset(&act.sa_mask);
    act.sa_flags=SA_NOCLDSTOP;
    
    res = sigaction(SIGCHLD, &act, NULL);
    if (res != 0) {
        elogmsg("sigaction()");
        exit(EXIT_FAILURE);
    }
    
    // unmask wanted signals
    
    res = sigdelset(oldset, SIGINT);
    if (res != 0) {
        elogmsg("sigdelset()");
        exit(EXIT_FAILURE);
    }
    res = sigdelset(oldset, SIGQUIT);
    if (res != 0) {
        elogmsg("sigdelset()");
        exit(EXIT_FAILURE);
    }
    res = sigdelset(oldset, SIGTERM);
    if (res != 0) {
        elogmsg("sigdelset()");
        exit(EXIT_FAILURE);
    }
    res = sigdelset(oldset, SIGCHLD);
    if (res != 0) {
        elogmsg("sigdelset()");
        exit(EXIT_FAILURE);
    }

    pthread_sigmask(SIG_SETMASK, oldset, NULL);
    if (res != 0) {
        elogmsg("pthread_sigmask()");
        exit(EXIT_FAILURE);
    }
}

/* see axiom-run.h */
void terminate_thread(pthread_t th, int endfd, char *logheader) {
    int res;
    if (endfd!=-1) {
        zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "%s: sending eventfd exit request to %ld", logheader, (long) th);
        res = eventfd_write(endfd,1);;
        if (res==-1) {
            // should not happen
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "%s: eventfd_write() error res=%d '%s'", logheader, res, strerror(res));
            endfd=-1;            
        }        
    } 
    if (endfd==-1) {
        zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "%s: sendind DEPRECATED cancellation request to %ld", logheader, (long) th);
        res = pthread_cancel(th);
        if (res != 0) {
            // it is normal to have an error if the thread has already died
            zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "%s: pthread_cancel() error res=%d '%s'", logheader, res, strerror(res));
            return;
        }
    }
    zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "%s: join %ld", logheader, (long) th);
    res = pthread_join(th, NULL);
    if (res != 0) {
        zlogmsg(LOG_WARN, LOGZ_SLAVE, "%s: pthread_joinl() error res=%d '%s'", logheader, res, strerror(res));
        return;
    }
    zlogmsg(LOG_DEBUG, LOGZ_SLAVE, "%s: joined %ld", logheader, (long) th);
}

#define min(x,y) ((x)<(y)?(x):(y))

extern char **environ;
/**
 * Prepare the environment.
 * 
 * The environemtn of the child application is built from teh environment of axiom-run.
 * For default it is composed by the variables PATH, SHELL and AXIOM_? but a regular expression can be passed
 * to select the environment.
 * Note that if a regexp is passed it is free at the end.
 * The AXIOM_RUN variable is added if on slave.
 *
 * @param env result (a list of environment variables)
 * @param re a regular expression to select the environment variables (can be null)
 * @param slave if this is the slave process
 * @param noclose if it is true, a null is not appended to result
 * @param nodes bitmap of nodes active in this session (bit0 = 1 if node0 is in the session)
 */
static void prepare_env(strlist_t *env, regex_t *re, int slave, int noclose, uint64_t nodes) {
    char **ptr = environ;
    int already = 0;
    regex_t myre;
    regmatch_t w;
    char *eq;
    int res;
    int sz;
    if (re == NULL) {
        regcomp(&myre, "PATH|SHELL|AXIOM_.*", REG_EXTENDED);
        re = &myre;
    }
    while (*ptr != NULL) {
        eq = strchr(*ptr, '=');
        if (eq != NULL) {
            sz = eq - *ptr;
            *eq = '\0';
            res = regexec(re, *ptr, 1, &w, 0);
            *eq = '=';
            if (res == 0 && w.rm_so == 0 && w.rm_eo == sz) {
                sl_append(env, *ptr);
            }
            if (strncmp("AXIOM_RUN", *ptr, 9) == 0) {
                zlogmsg(LOG_WARN, LOGZ_MAIN, "AXIOM_RUN environment variable already present!");
                already = 1;
            }
            if (strncmp("AXIOM_NODES", *ptr, 11) == 0) {
                zlogmsg(LOG_WARN, LOGZ_MAIN, "AXIOM_NODES environment variable already present!");
                already = 1;
            }
            /*
            if (strncmp("AXIOM_", *ptr, min(6, sz)) == 0) {
                sl_append(env, *ptr);
            } else if (sz == 4 && strncmp("PATH", *ptr, 4) == 0) {
                sl_append(env, *ptr);
            } else if (sz == 5 && strncmp("SHELL", *ptr, 5) == 0) {
                sl_append(env, *ptr);
            }
             */
        }
        ptr++;
    }
    if (slave&&!already) {
        char buf[64];
        /* count the number of slaves nodes */
        int nnodes = 0;
        uint64_t n = nodes;
        while (n != 0) {
            if (n & 0x1) nnodes++;
            n >>= 1;
        }
        //
        snprintf(buf, sizeof (buf), "AXIOM_RUN=%d", nnodes);
        sl_append(env, buf);
        snprintf(buf, sizeof (buf), "AXIOM_NODES=0x%lx", nodes);
        sl_append(env, buf);
    }
    if (!noclose) sl_append(env, NULL);
    regfree(re);
}

/**
 * Start the application on slave nodes using axiom_init service.
 *
 * @param dev the axiom device to use
 * @param master_port the port of the master
 * @param nodes the nodes bitwise (where to start application)
 * @param filename the exec filename
 * @param argv the args of the application
 * @param envp the environemnt of the application
 * @param gdb_nodes nodes bitwise where to run gdbserver. if zero gdb server in never run.
 * @param gdb_port gdb server port. unused if gdb_nodes is zero
 * @return AXIOM_RET_OK if "all" applications are started AXIOM_RET_ERROR otherwise
 */
static axiom_err_t start(axiom_dev_t *dev, int master_port, uint64_t nodes, char * filename, char * const argv[], char * const envp[], uint64_t gdb_nodes, int gdb_port) {
    axiom_err_t err = AXIOM_RET_OK;
    axiom_node_id_t node = 0;
    int counter = 0;
    char **gdb_argv;
    if (gdb_nodes != 0) {
        // TODO: check malloc!
        int sz = 0;
        while (argv[sz] != NULL) sz++;
        gdb_argv = malloc(sizeof (char*) *(3 + sz));
        gdb_argv[0] = "gdbserver";
        gdb_argv[1] = malloc(16);
        gdb_argv[2] = filename; // or gdb_argv[2] = argv[0] ?
        snprintf(gdb_argv[1], 16, "0.0.0.0:%d", gdb_port);
        memcpy(gdb_argv + 3, argv + 1, sizeof (char*)*sz);
    }
    while (nodes != 0) {
        if (nodes & 0x1) {
            if (gdb_nodes & 0x1) {
                err = axinit_execvpe(dev, master_port, node, 0, "gdbserver", gdb_argv, envp);
            } else {
                err = axinit_execvpe(dev, master_port, node, 0, filename, argv, envp);
            }
            if (!AXIOM_RET_IS_OK(err)) {
                zlogmsg(LOG_WARN, LOGZ_MAIN, "axinit_execvpe() error res=%d", err);
                break;
            }
            counter++;
        }
        nodes >>= 1;
        gdb_nodes >>= 1;
        node++;
    }
    zlogmsg(LOG_DEBUG, LOGZ_MAIN, "spawned application on %d nodes", counter);
    if (gdb_nodes != 0) {
        free(gdb_argv[1]);
        free(gdb_argv);
    }
    return err;
}

/** max node supported by the */
#define MAX_NODES_SUPPORTED 64

/*
 * MAX_NUM_NODES is semantically different from MAX_NODES_SUPPORTED
 *
 * MAX_NODES_SUPPORTED is the max number of nodes supported by the implementation of bitwise node mask
 * MAX_NUM_NODES is the maximu number of nodes supported by the system (for example to allocate statically buffers)
 */

#if MAX_NUM_NODES>MAX_NODES_SUPPORTED
#error MAX_NUM_NODES is > of MAX_NODES_SUPPORTED
#elif MAX_NUM_NODES==MAX_NODES_SUPPORTED
/** a bitwise for all nodes */
#define ALL_NODES 0xffffffffffffffff
#else
/** a bitwise for MAX_NUM_NODES nodes */
#define ALL_NODES (((uint64_t)1<<MAX_NUM_NODES)-1)
#endif

/** a bitwise for all active nodes nodes */
#define ALL_NODES_MASK(x) ((((x)>=MAX_NODES_SUPPORTED)?0xffffffffffffffff:(((uint64_t)1<<(x))-1)) << AXIOM_INIT_MASTER_NODE)

/**
 * Build the bitwise node mask from a command line parameter.
 * For example correct parameter are:
 * - 0,1,5 (nodes 0, 1 and 5)
 * - 0-3,7 (nodes 0, 1, 2, 3 and 7)
 * - 0-2,1-4,7,8 (nodes 0, 1, 2, 3, 4, 7 and 8)
 * - 0x5 (nodes 0 and 2)
 * - 0x38 (nodes 3, 4 and 5)
 *
 * @param arg the node parameter
 * @return the bitwsise node mask
 */
static uint64_t decode_node_arg(char *arg) {
    int n, a, b;
    uint64_t r = 0;
    char *s;
    if (strncmp(arg, "0x", 2) == 0) {
        n = sscanf(arg, "%lx", &r);
        if (n != 1) {
            usage();
            exit(EXIT_FAILURE);
        }
    } else {
        s = strtok(arg, ",");
        while (s != NULL) {
            if (strchr(s, '-') != NULL) {
                n = sscanf(s, "%d-%d", &a, &b);
                if (n != 2 || a < 0 || a >= b || b >= MAX_NUM_NODES) {
                    usage();
                    exit(EXIT_FAILURE);
                }
                if (b - a + 1 == MAX_NUM_NODES)
                    r = ALL_NODES;
                else
                    r |= ((((uint64_t) 1 << (b - a + 1)) - 1) << a);
            } else {
                n = sscanf(s, "%d", &a);
                if (n != 1 || a < 0 || a >= MAX_NUM_NODES) {
                    usage();
                    exit(EXIT_FAILURE);
                }
                r |= ((uint64_t) 1 << a);
            }
            s = strtok(NULL, ",");
        }
    }
    return r;
}
/**
 * Build the bitwise nodes mask for gdb from command line.
 *
 * For example correct parameter are:
 * - 0-3,7:1234 (nodes 0, 1, 2, 3 and 7 on port 1234)
 * - 0x5:4321 (nodes 0 and 2 on port 4321)
 *
 * @param arg the node and port parameter
 * @param nodes the bitwise node result
 * @param port the port result
 */
void decode_gdb_arg(char *arg, uint64_t *nodes, int *port) {
    char *s = strchr(arg, ':');
    if (s == NULL) {
        _usage("error on gdb arg: no semicolon found\n");
        exit(EXIT_FAILURE);
    }
    *s = '\0';
    *nodes = decode_node_arg(arg);
    *s = ':';
    *port = atoi(s + 1);
}

/**
 * Find the number of zero bits on the left of the parameter.
 * @param n the parameter
 * @return the number of ZERO bits on the left
 */
static inline int __clz(register uint64_t n) {
    // warning: GCC __builtin_clz() does not work with uint64_t
    register int c = 0;
    if (n == 0) return 64;
    while (n != 0) {
        if (n & 0x8000000000000000) break;
        n <<= 1;
        c++;
    }
    return c;
}

/**
 * Main axiom-run entry point.
 * @param argc number of argment
 * @param argv arguments
 * @return exit status
 */
int main(int argc, char **argv) {
    axiom_dev_t *dev = NULL;
    axiom_app_id_t app_id = AXIOM_NULL_APP_ID;
    int long_index = 0;
    int opt = 0;
    int port = -1;
    int slave = 0;
    int services = 0;
    uint64_t nodes = 0;
    uint64_t gdb_nodes = 0;
    int flags = 0;
    int gdb_port = 0;
    int envreg = 0;
    regex_t _envreg;
    int num_nodes;
    axiom_err_t err;
    strlist_t env;
    int exitval=0,myexit;
    int res;

    logmsg_init();
    zlogmsg(LOG_INFO, LOGZ_MAIN, "axiom-run started");

    //
    // command line parsing
    //

    while ((opt = getopt_long(argc, argv, "+rebcasp:m:hn:u:g:i::", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'u':
                envreg = 1;
                res = regcomp(&_envreg, optarg, REG_EXTENDED);
                if (res != 0) {
                    _usage("error on env flag: '%s' is not an extended regular expression\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r':
                services |= REDIRECT_SERVICE;
                break;
            case 'i':
                flags |= IDENT_FLAG;
                if (optind < argc && **(argv + optind) >= '0' && **(argv + optind) <= '9') {
                    optarg = *(argv + optind);
                    optind++;
                }
                if (optarg != NULL) {
                    int mode = atoi(optarg);
                    if (mode == 1) {
                        flags |= ALTERNATE_IDENT_FLAG;
                    }
                }
                break;
            case 'b':
                services |= BARRIER_SERVICE;
                break;
            case 'c':
                services |= RPC_SERVICE;
                break;
            case 'e':
                services |= EXIT_SERVICE;
                break;
            case 'a':
                services |= ALLOCATOR_SERVICE;
                break;
            case 'n':
                nodes = decode_node_arg(optarg);
                break;
            case 'g':
                decode_gdb_arg(optarg, &gdb_nodes, &gdb_port);
                break;
            case 's':
                slave = 1;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'm':
                if (strchr(optarg, ',') != NULL) {
                    sscanf(optarg, "%d,%d", &master_node, &master_port);
                } else {
                    master_node = atoi(optarg);
                }
                break;
            case 'h':
                usage();
                exit(-1);
            case '?':
            case ':':
                break;
            default:
                _usage("internal error on getopt_long()\n");
                exit(-1);
        }
    }

    //
    // some messages to log
    // (for debug purpose, only if enabled)
    //

    if (logmsg_is_zenabled(LOG_INFO, LOGZ_MAIN)) {
        char *s;
        int sz = 0;
        int idx = 0;
        while (idx < argc) {
            sz += strlen(argv[idx]);
            idx++;
        }
        sz += argc + 3;
        s = malloc(sz);
        if (s != NULL) {
            *s = '\0';
            idx = 0;
            while (idx < optind) {
                strncat(s, argv[idx], sz);
                strncat(s, " ", sz);
                idx++;
            }
            logmsg(LOG_INFO, "command line: %s", s);
            *s = '\0';
            idx = optind;
            while (idx < argc) {
                strncat(s, argv[idx], sz);
                strncat(s, " ", sz);
                idx++;
            }
            logmsg(LOG_INFO, "to run      : %s", s);
            free(s);
        }
    }

    //
    // parameters check
    //

    if (optind == argc) {
        fprintf(stderr, "error: APPLICATION name parameter is required\n");
        usage();
        exit(EXIT_FAILURE);
    }
    if (port != -1) {
        if (slave) slave_port = port;
        else master_port = port;
    }
    if (master_port == -1) {
        if (slave) {
            zlogmsg(LOG_DEBUG, LOGZ_MAIN, "I am a slave, master node port not set! using default");
        } else {
            zlogmsg(LOG_DEBUG, LOGZ_MAIN, "using default port for master");
        }
        master_port = MY_DEFAULT_MASTER_PORT;
    }
    if (slave_port == -1) {
        if (slave)
            zlogmsg(LOG_WARN, LOGZ_MAIN, "I am a slave, slave node port not set! using default");
        slave_port = MY_DEFAULT_SLAVE_PORT;
    }

    //
    // open/bind axiom device
    //

    if (services || !slave || gdb_nodes > 0) {

        // we need an axiom device only if:
        // - we have some services (communication between master and slave))
        // - I am the master (communication with axiom-init to start application)
        // - gdb enabled (we must know out node number)

        zlogmsg(LOG_DEBUG, LOGZ_MAIN, "opening/bindig axiom device");

        /* open the axiom device */
        dev = axiom_open(NULL);
        if (dev == NULL) {
            perror("axiom_open()");
            exit(EXIT_FAILURE);
        }

        // command line parameters check...
        if (!slave) {
            if (master_node == -1) {
                master_node = (int) axiom_get_node_id(dev);
            } else {
                int node = (int) axiom_get_node_id(dev);
                if (node != master_node) {
                    zlogmsg(LOG_WARN, LOGZ_MAIN, "I am a master, my node is %d BUT parameter for master node is %d", node, master_node);
                }
            }
        }

        /* bind to port */
        err = axiom_bind(dev, slave ? slave_port : master_port);
        if (!AXIOM_RET_IS_OK(err)) {
            perror("axiom_bind()");
            exit(EXIT_FAILURE);
        }

#if 0
        if (!slave) {
            axiom_err_t err = axiom_flush_raw(dev);
            if (!AXIOM_RET_IS_OK(err)) {
                zlogmsg(LOG_WARN, LOGZ_MAIN, "axiom_flush_raw() error %d", (int) err);
            }
        }
#endif
    }

    //
    // parameters check (continue)
    //

    if (master_node == -1) {
        if (slave) {
            zlogmsg(LOG_WARN, LOGZ_MAIN, "I am a slave, master node number not set! using default (%d)", MY_DEFAULT_MASTER_NODE);
            master_node = MY_DEFAULT_MASTER_NODE;
        } else {
            master_node = axiom_get_node_id(dev);
        }
    }
    if (!slave) {
        int bn;
        num_nodes = axiom_get_num_nodes(dev);
        int n = num_nodes;
        if (num_nodes > MAX_NUM_NODES) {
            zlogmsg(LOG_WARN, LOGZ_MAIN, "axiom-rum is using a max of %d nodes (we find %d nodes!)", MAX_NUM_NODES, num_nodes);
            n = MAX_NUM_NODES;
        }
        if (nodes == 0) {
            nodes = ALL_NODES_MASK(n);
        }
        bn = (MAX_NODES_SUPPORTED - __clz(nodes) - 1); // max node number (0-based)
        if (bn > n) {
            zlogmsg(LOG_WARN, LOGZ_MAIN, "crop max node number to %d", n);
            nodes &= ALL_NODES_MASK(n);
        }
        lassert(nodes != 0);
    }

    //
    // log info message
    //

    zlogmsg(LOG_INFO, LOGZ_MAIN, "mode: %s master_node=%d master_port=%d slave_port=%d services=0x%02x nodes=0x%016lx", slave ? "SLAVE" : "MASTER", master_node, master_port, slave_port, services, nodes);

    //
    // prepare environment child
    //

    sl_init(&env);
    prepare_env(&env, envreg ? &_envreg : NULL, slave, 1, nodes);
    if (slave && (services & (BARRIER_SERVICE|RPC_SERVICE))) {
        char var[128];
        snprintf(var, sizeof (var), "AXIOM_USOCK=%d", getpid());
        sl_append(&env, var);
    }

    if (!slave && (services & ALLOCATOR_SERVICE)) {
        int app_master = __builtin_ctzll(nodes); /* the smallest ID in the session is the app master */
        char var[128];

        /* send request and wait the reply for unique app-id */
        err = axiom_al2_alloc_appid(dev, master_port, &app_id);
        if (!AXIOM_RET_IS_OK(err)) {
            elogmsg("axiom_al2_alloc_appid()");
            exit(EXIT_FAILURE);
        }

        snprintf(var, sizeof(var), AXIOM_ENV_ALLOC_APPID"=%d", app_id);
        sl_append(&env, var);

        snprintf(var, sizeof(var), AXIOM_ENV_ALLOC_APPMASTER"=%d", app_master);
        sl_append(&env, var);
    }

    sl_append(&env, NULL);

    //
    // SLAVE or MASTER?
    //

    if (slave) {

        /*
         *
         * SLAVE MODE
         *
         */

        // argument for the exec call
        char *myexec = *(argv + optind);
        char **myargv = argv + optind;

        int rungdb = 0;
        int fd[3];
        pid_t pid;

        // test if we must run gdbserver...
        if (gdb_nodes != 0) {
            int mynodeid = (int) axiom_get_node_id(dev);
            if (gdb_nodes & (1 << mynodeid)) {
                rungdb = 1;
            }
        }

        // if we must run gdb we must change the arguments of the exec...
        if (!rungdb) {
            zlogmsg(LOG_DEBUG, LOGZ_MAIN, "slave mode enabled");
        } else {
            int sz = 0;
            zlogmsg(LOG_DEBUG, LOGZ_MAIN, "slave GDB mode enabled");
            myexec = "gdbserver";
            while (argv[sz + optind] != NULL) sz++;
            myargv = malloc(sizeof (char*) *(3 + sz));
            myargv[0] = "gdbserver";
            myargv[1] = malloc(16);
            myargv[2] = *(argv + optind);
            snprintf(myargv[1], 16, "0.0.0.0:%d", gdb_port);
            memcpy(myargv + 3, argv + optind + 1, sizeof (char*)*sz);
        }

        // fork/exec the child...
        pid = daemonize(NULL, myexec, myargv, sl_get(&env), (services & REDIRECT_SERVICE) ? fd : NULL, 0, 1);
        
        // release resources...
        if (rungdb) {
            free(myargv[1]);
            free(myargv);
        }

        // manage services....
        if (services) {
            exitval=manage_slave_services(dev, services, fd, pid);
        }

    } else {

        /*
         *
         * MASTER MODE
         *
         */

        // parameters ofr the exec...
        char *myfilename;
        char **myargs;
        char **myenv;

        if (!services) {
            zlogmsg(LOG_DEBUG, LOGZ_MAIN, "master mode enabled");
            myfilename = *(argv + optind);
            myargs = argv + optind;
            myenv = sl_get(&env);
        } else {
            //
            // if services are enabled we must run axiom-run on remote nodes...
            //
            strlist_t list;
            char **args = argv + optind;
            char buf[64];

            zlogmsg(LOG_DEBUG, LOGZ_MAIN, "master SERVICE mode enabled");
            sl_init(&list);
            sl_append(&list, argv[0]); // i.e. "axiom-run"
            sl_append(&list, "-s");
            sl_append(&list, "-n");
            snprintf(buf, sizeof (buf), "0x%lx", nodes);
            sl_append(&list, buf);
            sl_append(&list, "-p");
            snprintf(buf, sizeof (buf), "%d", slave_port);
            sl_append(&list, buf);
            sl_append(&list, "-m");
            snprintf(buf, sizeof (buf), "%d,%d", master_node, master_port);
            sl_append(&list, buf);
            sl_append(&list, "-u");
            sl_append(&list, ".*");
            if (services) {
                if (services & REDIRECT_SERVICE)
                    sl_append(&list, "-r");
                if (services & BARRIER_SERVICE)
                    sl_append(&list, "-b");
                if (services & EXIT_SERVICE)
                    sl_append(&list, "-e");
                if (services & RPC_SERVICE)
                    sl_append(&list, "-c");
            }

            if (gdb_nodes != 0) {
                sl_append(&list, "-g");
                snprintf(buf, sizeof (buf), "0x%lx:%d", gdb_nodes, gdb_port);
                sl_append(&list, buf);
            }

            sl_append_all(&list, args);
            sl_append(&list, NULL);

            myfilename = argv[0];
            myargs = sl_get(&list);
            myenv = sl_get(&env);
        }

        // start remote nodes process...
        err = start(dev, master_port, nodes, myfilename, myargs, myenv, (!services) ? gdb_nodes : 0, gdb_port);
        if (!AXIOM_RET_IS_OK(err)) {
            elogmsg("axinit_execve()");
            exit(EXIT_FAILURE);
        }

        // manage servicies...
        if (services) {
            exitval=manage_master_services(dev, services, nodes, flags, app_id);
        }
    }

    if (dev != NULL)
        axiom_close(dev);

    zlogmsg(LOG_INFO, LOGZ_MAIN, "axiom-run finished");

    /* see "Appendix E. Exit Codes With Special Meanings" in bash manual */
    myexit=WIFEXITED(exitval)?WEXITSTATUS(exitval):(WIFSIGNALED(exitval)?WTERMSIG(exitval)+128:255);
    zlogmsg(LOG_DEBUG, LOGZ_MAIN, "%s: exitval=0x%08x axiom-run.exit=%d",slave?"SLAVE":"MASTER",exitval,myexit);
    return myexit;
}

