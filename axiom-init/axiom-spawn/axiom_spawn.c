/*!
 * \file axiom_spawn.c
 *
 * This file contains the functions used in the axiom-init deamon to handle
 * the axiom-spawn messages.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "axiom_nic_types.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "dprintf.h"

#include "../axiom-init.h"

extern size_t strlcat(char *dst, const char *src, size_t size);
extern size_t strlcpy(char *dst, const char *src, size_t size);

/*
static inline char *_strncpy(char *dest, const char *src, size_t n) {
    char *res=strncpy(dest,src,n);
    dest[n-1]='\0';
    return res;
}
 */

/*
 * LIST MANAGER
 * to manage the strlist_t
 */

typedef struct strlist {
    int size;
    char **data;
} strlist_t;

static void sl_add(strlist_t *p, char *s) {
    void *ptr;
    if (p->size==0) {
        ptr=malloc(sizeof(char*));
    } else {
        ptr=realloc(p->data,sizeof(char*)*(p->size+1));
    }
    if (ptr==NULL) {
        EPRINTF("SPAWN - realloc() fail!");
        return;
    }
    p->data = (char**) ptr;
    p->size++;
    if (s==NULL) {
        p->data[p->size - 1] = NULL;
    } else {
        int size = strlen(s) + 1;
        char *s2 = malloc(size);
        if (s2==NULL) {
            EPRINTF("SPAWN - malloc() fail!");
            return;
        }
        strlcpy(s2, s, size);
        p->data[p->size - 1] = s2;
    }
}

static inline char** sl_get(strlist_t *p) {
    return p->data;
}

static void sl_init(strlist_t *p) {
    p->size = 0;
    p->data = NULL;
}

static void sl_free(strlist_t *p) {
    if (p->size>0) {
        int i;
        for (i=0;i<p->size;i++) {
            if (p->data[i]!=NULL) free(p->data[i]);
        }
        free(p->data);
    }
    sl_init(p);
}

/*
 * SPAWN manager
 */

#define MAX_CONCURRENT_SPAWNS 4

#define EMPTY_APPLICATION_ID 0

typedef struct {
    uint8_t application;
    char *exec;
    char *cwd;
    strlist_t args;
    strlist_t env;
} info_t;

static info_t info[MAX_CONCURRENT_SPAWNS];

void axiom_spawn_init() {
    int i;
    memset(info, 0, sizeof (info));
    // paranoia
    for (i=0;i<MAX_CONCURRENT_SPAWNS;i++) {
        info[i].application = EMPTY_APPLICATION_ID;
        info[i].exec = NULL;
        info[i].cwd = NULL;
        sl_init(&info[i].args);
        sl_init(&info[i].env);
    }
    //
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        EPRINTF("SPAWN - init error setting SIGCHLD to IGNORE... but continue...");
    }
}

static void run(info_t *info, int verbose, axiom_dev_t *dev);

void axiom_spawn_req(axiom_dev_t *dev, axiom_node_id_t src, axiom_payload_size_t payload_size, axiom_init_payload_t *_payload, int verbose) {

    axiom_spawn_req_payload_t *payload = ((axiom_spawn_req_payload_t *) _payload);
    int idx, idx_free;
    int size;

    if (payload->command != AXIOM_CMD_SPAWN_REQ) {
        EPRINTF("SPAWN - receive a not AXIOM_SPAWN_REQ message");
        return;
    }
    IPRINTF(verbose, "SPAWN - REQ message received - src_node: %u application: %u type: %u", src, payload->application, payload->type);
    
    for (idx = 0, idx_free = -1; idx < MAX_CONCURRENT_SPAWNS; idx++) {
        if (info[idx].application == EMPTY_APPLICATION_ID) idx_free = idx;
        if (info[idx].application == payload->application) break;
    }

    if (idx >= MAX_CONCURRENT_SPAWNS) {
        if (idx_free == -1) {
            DPRINTF("SPAWN - too much processo to spawn!");
            return;
        }
        idx = idx_free;
        info[idx].application = payload->application;
        payload->flags|=AXIOM_SPAWN_FLAG_RESET; // auto reset!!!
    }

    if (payload->flags & AXIOM_SPAWN_FLAG_RESET) {
        if (info[idx].exec != NULL) free(info[idx].exec);
        info[idx].exec = NULL;
        if (info[idx].cwd != NULL) free(info[idx].cwd);
        info[idx].cwd = NULL;
        sl_free(&info[idx].args);
        sl_free(&info[idx].env);
    }

    switch (payload->type) {
        case AXIOM_SPAWN_TYPE_EXE:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE - 1] = '\0'; // paranoia
            size = strlen((char*) payload->data) + 1;
            if (info[idx].exec != NULL) free(info[idx].exec);
            info[idx].exec = malloc(size);
            if (info[idx].exec == NULL) {
                EPRINTF("SPAWN - malloc() fail!");
            } else {
                strlcpy(info[idx].exec, (char*) payload->data, size);
            }
            break;
        case AXIOM_SPAWN_TYPE_CWD:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE - 1] = '\0'; // paranoia
            size = strlen((char*) payload->data) + 1;
            if (info[idx].cwd != NULL) free(info[idx].cwd);
            info[idx].cwd = malloc(size);
            if (info[idx].cwd == NULL) {
                EPRINTF("SPAWN - malloc() fail!");
            } else {
                strlcpy(info[idx].cwd, (char*) payload->data, size);
            }
            break;
        case AXIOM_SPAWN_TYPE_ARG:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE - 1] = '\0'; // paranoia
            sl_add(&info[idx].args, (char*) payload->data);
            break;
        case AXIOM_SPAWN_TYPE_ENV:
            payload->data[AXIOM_SPAWN_MAX_DATA_SIZE-1]='\0'; // paranoia
            sl_add(&info[idx].env,(char*)payload->data);
            break;
        default:
            EPRINTF("SPAWN - unknow type %u", payload->type);
            break;
    }

    if (payload->flags & AXIOM_SPAWN_FLAG_EXEC) {
        sl_add(&info[idx].args, NULL);
        sl_add(&info[idx].env, NULL);
        //
        run(&info[idx], verbose, dev);
        //
        info[idx].application = EMPTY_APPLICATION_ID;
        sl_free(&info[idx].env);
        sl_free(&info[idx].args);
        if (info[idx].exec != NULL) {
            free(info[idx].exec);
            info[idx].exec = NULL;
        }
        if (info[idx].cwd != NULL) {
            free(info[idx].cwd);
            info[idx].cwd = NULL;
        }
    }

}

/*
static void *waitchild(void *arg) {
    pid_t pid = (pid_t) (intptr_t) arg;
    int status, ret;
    for (;;) {
        ret = waitpid(pid, &status, 0);
        if (ret == -1) {

        }
        if ((!WIFSTOPPED(status))&&(!WIFCONTINUED(status)) && ret != -1) break;
    }
    return NULL;
}
 */

static void run(info_t *p, int verbose, axiom_dev_t *dev) {
    pid_t pid;

    if (verbose) {
        char **ptr;
        IPRINTF(verbose, "SPAWN");
        IPRINTF(verbose, "  exe=%s", p->exec);
        IPRINTF(verbose, "  cwd=%s", p->cwd);
        ptr = sl_get(&p->args);
        while (*ptr != NULL) {
            IPRINTF(verbose, "  arg=%s", *ptr);
            ptr++;
        }
        ptr = sl_get(&p->env);
        while (*ptr != NULL) {
            IPRINTF(verbose, "  env=%s", *ptr);
            ptr++;
        }
    }

    pid=fork();
    if (pid<0) {
        EPRINTF("axiom spawn - fork() error!");
        return;
    }
    if (pid != 0) {
        // NOTHING TO DO!
        /*
        pthread_attr_t attr;
        pthread_t th;
        int res;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        res = pthread_create(&th, &attr, &waitchild, (void*) (intptr_t) pid);
        pthread_attr_destroy(&attr);
        if (res != 0) {
            EPRINTF("axiom spawn - waitchild thread creation failed!");
        }
         */
    } else {
        char *ifile, *ofile;
        int res;
        pid_t sid;
        int i, fd;
        //
        // close axiom port
        // needed???
        //
        axiom_close(dev);
        //
        // change working directory
        //
        if (p->cwd != NULL) {
            res = chdir(p->cwd);
            if (res != 0) {
                // ignored
            }
        }
        //
        // new control terminal
        //
        sid = setsid();
        if (sid < 0) {
            // ignored
        }
        //
        // new stdin/stdout/stderr
        //
        ifile="/dev/null";
        ofile = "/dev/null";
        /*
        {
            ifile=malloc(PATH_MAX);
            snprintf(ifile, PATH_MAX, "/tmp/spawn.%d.in", getpid());
            ofile=malloc(PATH_MAX);
            snprintf(ofile, PATH_MAX, "/tmp/spawn.%d.out", getpid());
            fd = open(ifile, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
         */
        fd = open(ifile, 0);
        if (fd != -1) {
            dup2 (fd, STDIN_FILENO);
        } else {
            EPRINTF("axiom spawn - open of new stdin failed!");
        }
        fd = open(ofile, O_RDWR, 0);
        if (fd != -1) {
            dup2 (fd, STDOUT_FILENO);
            dup2 (fd, STDERR_FILENO);
        } else {
            EPRINTF("axiom spawn - open of new stdout/stderr failed!");
        }
        //
        // close unused file descriptor
        //
        for (i=getdtablesize(); i>=0; --i) {
            if (i!=STDIN_FILENO&&i!=STDOUT_FILENO&&i!=STDERR_FILENO)
                close(i);
        }
        // exec
        execve(p->exec, sl_get(&p->args), sl_get(&p->env));
        // exit... in case of exec failure
        EPRINTF("SPAWN - exec() failure (errno=%d %s)!", errno, strerror(errno));
        exit(0);
    }
}

