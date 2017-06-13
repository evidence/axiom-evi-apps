/*!
 * \file daemonize.c
 *
 * \version     v0.13
 *
 * The daemonize() utility function.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
// to use the execvpe() function
#define _GNU_SOURCE
#include <features.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "axiom_common.h"

extern char **environ;

/**
 * Clean (in case of failure).
 */
#define CLEAN() {\
  if (fdout!=-1) close(fdout);\
  if (fdin!=-1) close(fdin);\
  if (fderr!=-1) close(fderr);\
  if (pipefd!=NULL) {\
    if (pipefd[0]!=-1) close(pipefd[0]);\
    if (pipefd[1]!=-1) close(pipefd[1]);\
    if (pipefd[2]!=-1) close(pipefd[2]);\
  }\
}

/* See axiom_common.h */
pid_t daemonize(char *cwd, char *exec, char **args, char **env, int *pipefd, int newsession, int verbose) {

    pid_t pid;
    char *oldcwd;
    int fdout = -1, fdin = -1, fderr = -1;

    if (verbose && logmsg_is_enabled(LOG_INFO)) {
        char **ptr;
        logmsg(LOG_INFO, "daemonize() parameters");
        logmsg(LOG_INFO, "  exe=%s", exec);
        logmsg(LOG_INFO, "  cwd=%s", cwd);
        if (args != NULL) {
            ptr = args;
            while (*ptr != NULL) {
                logmsg(LOG_INFO, "  arg=%s", *ptr);
                ptr++;
            }
        }
        if (env != NULL) {
            ptr = env;
            while (*ptr != NULL) {
                logmsg(LOG_INFO, "  env=%s", *ptr);
                ptr++;
            }
        }
    }

    if (pipefd != NULL) {
        pipefd[0] = pipefd[1] = pipefd[2] = -1;
        int fd[2];
        // fd[0] read end
        // fd[1] write end
        if (pipe(fd) != 0) {
            elogmsg("pipe() for stdout failure");
            CLEAN();
            return -1;
        }
        fdout = fd[1];
        pipefd[1] = fd[0];
        if (pipe(fd) != 0) {
            elogmsg("pipe() for stdin failure");
            CLEAN();
            return -1;
        }
        fdin = fd[0];
        pipefd[0] = fd[1];
        if (pipe(fd) != 0) {
            elogmsg("pipe() for stderr failure");
            CLEAN();
            return -1;
        }
        fderr = fd[1];
        pipefd[2] = fd[0];
    }

    oldcwd = NULL;
    if (cwd != NULL) {
        int res;
        oldcwd = getcwd(NULL, 0);
        if (oldcwd == NULL) {
            elogmsg("getcwd() failure");
            CLEAN();
            return -1;
        }
        res = chdir(cwd);
        if (res != 0) {
            elogmsg("chdir() failure");
            CLEAN();
            return -1;
        }
    }

    pid = fork();
    if (pid != 0) {
        //
        // CURRENT PROCESS
        //
        if (oldcwd != NULL) {
            int res = chdir(oldcwd);
            if (res != 0) {
                logmsg(LOG_WARN, "daemonize() - chdir() error! (errno=%d %s)!", errno, strerror(errno));
            }
        }
        if (fdin != -1) close(fdin);
        if (fdout != -1) close(fdout);
        if (fderr != -1) close(fderr);
        if (pid < 0) {
            // no new process
            elogmsg("fork() failure");
            return -1;
        } else {
            // new process created
            // nothing to do
        }
    } else {
        //
        // NEW PROCCESS
        //
        char *nullargs[] = {exec, NULL};
        pid_t sid;
        int i, fd;
        //
        // new control terminal
        // setsid() -> become a session leader (and create a new process group) 
        if (newsession) {
            sid = setsid();
            if (sid < 0) {
                // ignored
            }
        }
        //
        // new stdin/stdout/stderr
        //
        fd = fdin == -1 ? open("/dev/null", O_RDONLY, 0) : fdin;
        if (fd != -1) {
            dup2(fd, STDIN_FILENO);
        } else {
            logmsg(LOG_WARN, "daemonize() - open of new stdin failed! (errno=%d %s)!", errno, strerror(errno));
        }
        fd = fdout == -1 ? open("/dev/null", O_WRONLY, 0) : fdout;
        if (fd != -1) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
        } else {
            logmsg(LOG_WARN, "daemonize() - open of new stdout failed! (errno=%d %s)!", errno, strerror(errno));
        }
        fd = fderr == -1 ? open("/dev/null", O_WRONLY, 0) : fderr;
        if (fd != -1) {
            dup2(fd, STDERR_FILENO);
        } else {
            logmsg(LOG_WARN, "daemonize() - open of new stderr failed! (errno=%d %s)!", errno, strerror(errno));
        }
        //
        // close unused file descriptor
        //
        for (i = getdtablesize(); i >= 0; --i) {
            if (i != STDIN_FILENO && i != STDOUT_FILENO && i != STDERR_FILENO)
                close(i);
        }
        // exec
        if (exec!=NULL) {
            execvpe(exec, args == NULL || *args == NULL ? nullargs : args, env == NULL ? environ : env);
            // exit... in case of execvpe failure
            elogmsg("execvpe() failure");
            exit(EXIT_FAILURE);
        }
    }

    return pid;
}
