/*!
 * \file logmsg.c
 *
 * \version     v0.12
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/param.h>

#include "axiom_common.h"

extern char **environ;

char *logmsg_name[] = {"FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};

static FILE *logmsg_fout = NULL; // :-( stderr is not constant
pid_t logmsg_pid = -1;
int logmsg_level = LOG_NOLOG;
int logmsg_zones = 0xffffffff;

void logmsg_to(char *filename) {
    // :-(
    FILE *old;
    FILE *f = fopen(filename, "w+");
    if (f == NULL) {
        //elogmsg("fopen()");
        return;
    }
    old = logmsg_fout;
    logmsg_fout = f;
    if (old != stderr) fclose(old);
    setvbuf(logmsg_fout, NULL, _IONBF, 0);
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void _logmsg(char *msg, ...) {
    va_list list;
    va_start(list, msg);
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_mutex_lock(&mutex);
    // NMB: logmsg_fout can be NULL if logmsg_init() has not been never called
    vfprintf(logmsg_fout, msg, list);
    pthread_mutex_unlock(&mutex);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    va_end(list);
}

void _slogmsg(char *msg, ...) {
    char buffer[256];
    char *p;
    int h, sz, r;
    int _errno;

    _errno = errno;

    va_list list;
    va_start(list, msg);
    // NMB: logmsg_fout can be NULL if logmsg_init() has not been never called
    vsnprintf(buffer, sizeof (buffer), msg, list);
    va_end(list);

    buffer[255] = '\0';
    h = fileno(logmsg_fout);
    sz = strlen(buffer);
    p = buffer;
    while (sz > 0) {
        r = write(h, p, sz);
        if (r <= 0) break;
        p += r;
        sz -= r;
    }

    errno = _errno;
}

void logmsg_init() {
    char buf[MAXPATHLEN];
    char *value;
    //
    value = getenv("AXIOM_LOG_LEVEL");
    if (value != NULL) {
        int i;
        for (i = 0; i<sizeof (logmsg_name) / sizeof (char*); i++) {
            if (strcasecmp(logmsg_name[i], value) == 0) {
                logmsg_level = i;
                break;
            }
        }
        if (i==sizeof (logmsg_name) / sizeof (char*)) {
            fprintf(stderr,"WARNING..... unknwon AXIOM_LOG_LEVEL value %s\n",value);
        }
    }
    //
    value = getenv("AXIOM_LOG_ZONES");
    if (value != NULL) {
        sscanf(value, "%d", &logmsg_zones);
    }
    //
    logmsg_fout = stderr;
    value = getenv("AXIOM_LOG_FILE");
    if (value!=NULL&&strstr(value,"%ld")!=NULL) {
        snprintf(buf,sizeof(buf),value,(long)getpid());
        value=buf;
    }
    if (value != NULL) {
        logmsg_to(value);
    }
    //
    logmsg_pid = getpid();
}

