
#include <linux/sched.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define gettid() syscall(SYS_gettid)

struct sched_attr {
    __u32 size;

    __u32 sched_policy;
    __u64 sched_flags;

    /* SCHED_NORMAL, SCHED_BATCH */
    __s32 sched_nice;

    /* SCHED_FIFO, SCHED_RR */
    __u32 sched_priority;

    /* SCHED_DEADLINE (nsec) */
    __u64 sched_runtime;
    __u64 sched_deadline;
    __u64 sched_period;
};

static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{
    return syscall(SYS_sched_setattr, pid, attr, flags);
}

void sch_usage(FILE *out)
{
    fprintf(out, "-S, --sched SCHED[:P_0:[:P_1:[:P_2]]\n");
    fprintf(out, "    set the scheduling parameters for the threads; SCHED can be (see 'sched' man page):\n");
    fprintf(out, "    OTHER    = use SCHED_OTHER (default if no --sched)\n");
    fprintf(out, "    BATCH    = use SCHED_BATCH\n");
    fprintf(out, "    IDLE     = use SCHED_IDLE\n");
    fprintf(out, "    FIFO     = use SCHED_FIFO\n");
    fprintf(out, "    RR       = use SCHED_RR (default if --sched without parameters; with P_0 equals to 1)\n");
    fprintf(out, "    DEADLINE = use SCHED_DEADLINE\n");
    fprintf(out, "    the optional parameters P_0,P_1 and P_2 are numbers that has different meanings for different SCHED:\n");
    fprintf(out, "      IDLE         has no optional parameters\n");
    fprintf(out, "      BATCH, OTHER P_0 is the nice value (default is 0)\n");
    fprintf(out, "      FIFO, RR     P_0 is the priority value (default is 1)\n");
    fprintf(out, "      DEADLINE     P_0 is the mean execution time, P_1 is the deadline, P_2 is the periond\n");
    fprintf(out, "    (for P_0 use 'chrt -m' to see parameter range and 'man sched' for more information)\n");
    fprintf(out, "    (for DEADLINE you should use 'u' for usec, 'm' for msec and none for sec)\n");
}

/* used for thread scheduling */
static int sched_policy = SCHED_OTHER;
static uint64_t sched_p0 = 0;
static uint64_t sched_p1 = 0;
static uint64_t sched_p2 = 0;

int sch_encodeopt(char *buf, int bufsize) {
    switch (sched_policy) {
        case SCHED_IDLE:
            snprintf(buf,bufsize,"%d",sched_policy);
            break;
        case SCHED_OTHER:
        case SCHED_BATCH:
        case SCHED_FIFO:
        case SCHED_RR:
            snprintf(buf,bufsize,"%d,%lu",sched_policy,sched_p0);
            break;
        case SCHED_DEADLINE:
            snprintf(buf,bufsize,"%d,%luu,%luu,%luu",sched_policy,sched_p0,sched_p1,sched_p2);
            break;
    }
    return 0;
}

int sch_decodeopt(char *optarg, void(*_usage)(const char *, ...))
{
    if (optarg != NULL) {
        int per = 0;
        char *start, *end;
        start = optarg;
        end = NULL;
        sched_policy = (int) strtol(start, &end, 10);
        if (start == end) {
            if (strncmp(start, "OTHER", 5) == 0) {
                sched_policy = SCHED_OTHER;
                end += 5;
            } else
                if (strncmp(start, "DEADLINE", 8) == 0) {
                sched_policy = SCHED_DEADLINE;
                end += 8;
            } else
                if (strncmp(start, "FIFO", 4) == 0) {
                sched_policy = SCHED_FIFO;
                end += 4;
            } else
                if (strncmp(start, "BATCH", 5) == 0) {
                sched_policy = SCHED_BATCH;
                end += 5;
            } else
                if (strncmp(start, "IDLE", 4) == 0) {
                sched_policy = SCHED_IDLE;
                end += 4;
            } else
                if (strncmp(start, "RR", 2) == 0) {
                sched_policy = SCHED_RR;
                end += 2;
            } else {
                if (_usage!=NULL) {
                    _usage("'%s': unrecognized policy name or value\n", optarg);
                }
                return -1;
            }
        }
        if (*end != '\0') {
            if (*end != ',') {
                if (_usage!=NULL) {
                    _usage( "'%s': comma expected (1)\n", optarg);
                }
                return -1;
            }
            start = end + 1;
            sched_p0 = strtol(start, &end, 10);
            if (start == end) {
                if (_usage!=NULL) {
                    _usage("'%s': unrecognized param value (1)\n", optarg);
                }
                return -1;
            }
           if (sched_policy==SCHED_DEADLINE) {
               if (*end=='u'||*end=='m'||*end=='%') {
                   if (*end=='m') sched_p0*=1000;
                   if (*end=='%') per=1;
                   end++;
               } else {
                   sched_p0*=1000*1000;
               }
           }
        } else {
            // DEFAULT VALUES
            switch (sched_policy) {
                case SCHED_IDLE:
                    break;
                case SCHED_OTHER:
                case SCHED_BATCH:
                    sched_p0 = 0;
                    break;
                case SCHED_FIFO:
                case SCHED_RR:
                    sched_p0 = 1;
                    break;
                case SCHED_DEADLINE:
                    // dummy :-(
                    // usec
                    sched_p0 = 100*1000;
                    sched_p1 = 995*1000;
                    sched_p2 = 1000*1000;
                    break;
            }
        }
        if (*end != '\0') {
            if (*end != ',') {
                if (_usage!=NULL) {
                    _usage("'%s': comma expected (2)", optarg);
                }
                return -1;
            }
            start = end + 1;
            sched_p1 = strtol(start, &end, 10);
            if (start == end) {
                if (_usage!=NULL) {
                    _usage("'%s': unrecognized param value (2)", optarg);
                }
                return -1;
            }
            if (sched_policy==SCHED_DEADLINE) {
                if (*end=='u'||*end=='m') {
                    if (*end=='m') sched_p1*=1000;
                    end++;
                } else {
                    sched_p1*=1000*1000;
                }
            }
        }
        if (*end != '\0') {
            if (*end != ',') {
                if (_usage!=NULL) {
                    _usage("'%s': comma expected (3)", optarg);
                }
                return -1;
            }
            start = end + 1;
            sched_p2 = strtol(start, &end, 10);
            if (start == end) {
                if (_usage!=NULL) {
                    _usage("'%s': unrecognized param value (3)", optarg);
                }
                return -1;
            }
            if (sched_policy==SCHED_DEADLINE) {
                if (*end=='u'||*end=='m') {
                    if (*end=='m') sched_p2*=1000;
                    end++;
                } else {
                    sched_p2*=1000*1000;
                }
            }
        }
        if (*end != '\0') {
            if (_usage!=NULL) {
                _usage("'%s': unexpected chars after parameter", optarg);
            }
            return -1;
        }

        if (per) {
            sched_p0=sched_p0*sched_p2/100;
        }
    }
    return 0;
}

int sch_setsched()
{
    struct sched_attr attr;
    pid_t mytid = gettid();
    memset(&attr, 0, sizeof (struct sched_attr));
    attr.size = sizeof (struct sched_attr);
    attr.sched_policy = sched_policy;
    switch (sched_policy) {
        case SCHED_IDLE:
            break;
        case SCHED_OTHER:
        case SCHED_BATCH:
            attr.sched_nice = sched_p0;
            break;
        case SCHED_FIFO:
        case SCHED_RR:
            attr.sched_priority = sched_p0;
            break;
        case SCHED_DEADLINE:
            attr.sched_runtime = sched_p0 * 1000;
            attr.sched_deadline = sched_p1 * 1000;
            attr.sched_period = sched_p2 * 1000;
            break;
    }
    return sched_setattr(mytid, &attr, 0);
}
