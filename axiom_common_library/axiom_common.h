/*!
 * \file axiom_common.h
 *
 * \version     v0.15
 *
 * Common internal functions.
 * A collections of functions used by some axiom-evi application.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_COMMON_H
#define AXIOM_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

    /* */
    /* */
    /* simple logging functions */
    /* */
    /* */

#include <sys/time.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

    /* If compiled with NLOG the logging subsystem will be discharged. 
     * Note that the overhead of not disabling is a machine test istruction for every possibly message to emit.
     * (so should be irrilevant).
     */

#ifdef NLOG

#include <assert.h>

#define logmsg_is_enabled(lvl) 0
#define logmsg_is_zenabled(lvl,zone) 0
#define zlogmsg(lvl, zone, msg, ...)
#define logmsg(lvl, msg, ...)
#define  _logmsg(lvl, msg, ...)
#define logmsg_init()
#define logmsg_to(pathanme)
#define logmsg_to_console()
#define lassert(x) assert(x)
#define elogmsg(msg, ...) {\
  fprintf(stderr,"(file=%s line=%d errno=%d '%s') " msg, __FILENAME__ , __LINE__ , errno, strerror(errno), ##__VA_ARGS__);\
}

#else

    /**
     * Level of logging.
     */
    typedef enum {
        LOG_NOLOG = -1/**< No logging. No emit log messages.*/,
        LOG_FATAL = 0, /**< Fatal. Error unrecoverable. .*/
        LOG_ERROR = 1, /**< Error. Internal error (recoverable). */
        LOG_WARN = 2, /**< Warning. Some information to notice. */
        LOG_INFO = 3, /**< Info. Information on important events. */
        LOG_DEBUG = 4, /**< Debug. Debug output.*/
        LOG_TRACE = 5 /**< Trace. Very verbose */
    } log_level_t;

    extern char *logmsg_name[];
    extern int logmsg_level;
    extern pid_t logmsg_pid;
    extern int logmsg_zones;

    /**
     * Emit a log message.
     * Used internally.
     * @param msg printf syle message.
     * @param ... parameters for the printf.
     */
    void _logmsg(char *msg, ...) __attribute__((format(printf, 1, 2)));

    /**
     * Emit a safe log message.
     * Safe to be used on a signal handler. Used internally.
     *
     * @param msg printf style message.
     * @param ... parameters for the printf.
     */
    void _slogmsg(char *msg, ...) __attribute__((format(printf, 1, 2)));

    /**
     * Test if a log level is enabled.
     * @param lvl the level to test
     * @return 1 if enabled.
     */
#define logmsg_is_enabled(lvl) ((lvl)<=logmsg_level)

    /**
     * Test if a log level is enabled on a zone.
     * @param lvl the level to test
     * @zone the zone to test
     */
#define logmsg_is_zenabled(lvl,zone) ((logmsg_zones&(zone))&&logmsg_is_enabled(lvl))


    /**
     * Emit a log message if enabled level/zone is enabled.
     * Emit a log message for a level on a zone only if enabled.
     * @param lvl the level
     * @param zone the zone
     * @param msg the printf style message
     * @param ... the parameters for the printf
     */
#define zlogmsg(lvl, zone, msg, ...) {\
  if (logmsg_is_zenabled(lvl,zone)) {\
    struct timespec _t0;\
    clock_gettime(CLOCK_REALTIME_COARSE,&_t0);\
    _logmsg("[%5d.%06d] %5s{%d}: " msg "\n", (int)(_t0.tv_sec % 10000), (int)_t0.tv_nsec/1000, logmsg_name[lvl], logmsg_pid, ##__VA_ARGS__);\
  }\
}

    /**
     * Emit a safe log messgae if enabled.
     * Emit a log message for a level on a zone only if enabled.
     * Can be used on  signal handler function.
     * @param lvl the level
     * @param zone the zone
     * @param msg the printf style message
     * @param ... the parameters for the printf
     */
#define szlogmsg(lvl, zone, msg, ...) {\
  if (logmsg_is_zenabled(lvl,zone)) {\
    struct timespec _t0;\
    clock_gettime(CLOCK_REALTIME_COARSE,&_t0);\
    _slogmsg("[%5d.%06d] %5s{%d}: " msg "\n", (int)(_t0.tv_sec % 1000), (int)_t0.tv_nsec/1000, logmsg_name[lvl], logmsg_pid, ##__VA_ARGS__);\
  }\
}

    /**
     * Emit a log message if level is enabled.
     * Emit in all zones.
     * @param lvl the log level
     * @param msg the printf style message
     * @param ... the parameters for the printf
     */
#define logmsg(lvl, msg, ...)  zlogmsg(lvl, 0xffffffff, msg, ##__VA_ARGS__)

    /**
     * Emit a safe log message if level is enabled.
     * Emit in all zones. Can be used froma signal handler.
     * @param lvl the log level
     * @param msg the printf style message
     * @param ... the parameters for the printf
     */
#define slogmsg(lvl, msg, ...)  szlogmsg(lvl, 0xffffffff, msg, ##__VA_ARGS__)

    /**
     * The __FILE__ with path stripped.
     */
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    /**
     * Emit an error log message.
     * Emit a log message. If LOG_ERROR level is disabled emit teh same message on stderr.
     */
#define elogmsg(msg, ...) {\
  logmsg(LOG_ERROR,"(file=%s line=%d errno=%d '%s') " msg, __FILENAME__ , __LINE__ , errno, strerror(errno), ##__VA_ARGS__);\
  if (!logmsg_is_enabled(LOG_ERROR)) fprintf(stderr,"(file=%s line=%d errno=%d '%s') " msg, __FILENAME__ , __LINE__ , errno, strerror(errno), ##__VA_ARGS__);\
}

    /**
     * Force to use the console for log messages.
     */
#define logmsg_to_console() logmsg_to("/dev/console")

    /**
     * Logging system initialization.
     * Must be called athe the beginning to init the log subsystem.
     *
     * Defaults:
     * - using stderr
     * - log level to LOG_NOLOG
     * - all zones enabled
     *
     * Some environment variables can be used to change the default:
     * - AXIOM_LOG_LEVEL the log level to enable (log_level_t without the LOG_ part)
     * - AXIOM_LOG_ZONE the zones to enabled (integer bitwise)
     * - AXIOM_LOG_FILE the file where to emit mlog messages
     */
    void logmsg_init();

    /**
     * Redirect the log output to a file.
     * @param pathanme The file to log into.
     */
    void logmsg_to(char *pathanme);

#ifdef NDEBUG
    /**
     * Assert macro.
     * Call standard assert().
     * @param x the assertion.
     */
#define lassert(x) {\
  if (!(x)) {\
     logmsg(LOG_WARN,"assertion '" #x "' FAILED!");\
  }\
}
#else
    /**
     * Assert macro with message to teh logging subsystem.
     * @param x the assertion.
     */
#define lassert(x) {\
  if (!(x)) {\
     logmsg(LOG_FATAL,"assertion '" #x "' FAILED!");\
     if (!logmsg_is_enabled(LOG_FATAL)) fprintf(stderr,"assertion '" #x "' FAILED!\n");\
     abort();\
  }\
}
#endif

#endif

    /* */
    /* */
    /* string functions */
    /* */
    /* */

#include <stddef.h>

    /**
     * strcat() with safe ending test.
     * Appends src to string dst of size dsize (unlike strncat, dsize is the
     * full size of dst, not space left).  At most dsize-1 characters
     * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
     * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
     * If retval >= dsize, truncation occurred.
     * Source from OpenBSD (C) 1998,2015 Todd C. Miller
     *
     * @param dst the destination string.
     * @param src the source string.
     * @param dsize the max destination size.
     * @return the destination string.
     */
    size_t strlcat(char *dst, const char *src, size_t dsize);

    /**
     * strcpy() with safe ending test.
     * Copy src to string dst of size siz.  At most siz-1 characters
     * will be copied.  Always NUL terminates (unless siz == 0).
     * Returns strlen(src); if retval >= siz, truncation occurred.
     * Source from OpenBSD (C) 1998 Todd C. Miller
     *
     * @param dst
     * @param src
     * @param size
     * @return
     */
    size_t strlcpy(char *dst, const char *src, size_t size);

    /* */
    /* */
    /* string list functions */
    /* */
    /* */

    /** A structure to implement a variable list of string. */
    typedef struct strlist {
        int size; /**< Size of data (i.e. how many pointer it has) */
        char **data; /**< Array of pointers to string.*/
    } strlist_t;

    /**
     * Initialize a strlist_t struncture.
     * @param p The structure to initialize.
     */
    void sl_init(strlist_t *p);

    /**
     * Insert a new string at the beginning of the list.
     * @param p The list.
     * @param s The string to add.
     */
    void sl_insert(strlist_t *p, char *s);

    /**
     * Insert a new string at the end of the list.
     * @param p The list.
     * @param s The string to add.
     */
    void sl_append(strlist_t *p, char *s);

    /**
     * Insert all the string to the head of the list.
     * @param p The list.
     * @param args The array of string (last element MUST BE a NULL).
     */
    void sl_insert_all(strlist_t *p, char **args);

    /**
     * Insert all the string to the end of the list.
     * @param p The list.
     * @param args The array of string (last element MUST BE a NULL).
     */
    void sl_append_all(strlist_t *p, char **args);

    /**
     * Return the list as as pointer.
     * Note that the returned array pointer is NOT null terminated.
     * 
     * @param p The list.
     * @return The pointer of the string array.
     */
    char** sl_get(strlist_t *p);

    /**
     * Release the resource used by the string list.
     * @param p The list.
     */
    void sl_free(strlist_t *p);

    /* */
    /* */
    /* running functions */
    /* */
    /* */

#include <sys/types.h>

    /**
     * Exec a program in background.
     * If pipefs pointer is not null 3 pipes for stdin, stdout and stderr are made and returned.
     * The semantic of exec, args, env are the same of execvpe().
     * If 'exec' is NULL then no new program is executed so the function return a pid_t of zero into the new daemonized process.
     * So if 'exec' is NULL the 'args' and 'env' parameters are ignored.
     *
     * @param cwd The working directory where to run (can be null).
     * @param exec The executable (can be null).
     * @param args The executable arguments.
     * @param env The executable environment.
     * @param pipefd A pointer to int[3] for pipes (can be null).
     * @param newsession if true create a new session/new process group
     * @param verbose Emit log message of "what are you doing?"
     * @return The pid of the new process or 0 in the new process (only if exec is NULL this is returned into the new process) or -1 in case of failure (set errno).
     */
    pid_t daemonize(char *cwd, char *exec, char **args, char **env, int *pipefd, int newsession, int verbose);

#ifdef __cplusplus
}
#endif

#endif

