/*!
 * \file axiom-ethtap.c
 *
 * \version     v1.2
 *
 * Tun/Tap user space program to utilize the tun/tap linux network device on top of Axiom driver.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <stdarg.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>
#include <ifaddrs.h>

#include "axiom_nic_api_user.h"
#include "axiom_nic_limits.h"

#include "axiom_common.h"

/*
 * 
 */
//#define TUN_MODE
#define TAP_MODE

#if defined(TUN_MODE)
#if defined(TAP_MODE)
#error Defined both TUN_MODE and TAP_MODE
#endif
#else
#if !defined(TAP_MODE)
#warning No TUN_MODE or TAP_MODE defined! TUN_MODE will be used!
#define TAP_MODE
#endif
#endif

/*
 * Constants
 */

#ifdef TUN_MODE
#error No implementation yet
#endif

static char const MAC[]={0x42,0xD9,0xAF,0x86,0x0B,0x34};
static char const BROADCAST[]={0xff,0xff,0xff,0xff,0xff,0xff};
#define PORT 2
#define MAX_THREADS 64


#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACVAL(p) *(p),*((p)+1),*((p)+2),*((p)+3),*((p)+4),*((p)+5)

/*
 * Global variables
 */

/** Axiom device. */
static axiom_dev_t *dev = NULL;
/** File handle to tun/tap driver. */
static int tunh = -1;
/** Number of axiom nodes. */
static int num_nodes;
/** My node number (ONE-based).*/
static int my_node;
/** My MAC address. */
static char mymac[]={0x00,0x00,0x00,0x00,0x00,0x00};
/** Axiom port used for comunication. */
static int port = PORT;

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
        fputc('\n', stderr);
    }
    fprintf(stderr, "usage: axiom-ethtap [ARG]*\n");
    fprintf(stderr, "Create a virtual ethernet device on top of AXIOM device,\n");
    fprintf(stderr, "Version: %s\n", AXIOM_API_VERSION_STR);
    fprintf(stderr, "\n\n");
    fprintf(stderr, "ARGs are:\n");
    fprintf(stderr, "-h, --help\n");
    fprintf(stderr, "    print this help screen and then exit\n");
    fprintf(stderr, "-f, --foreground\n");
    fprintf(stderr, "    run in foreground (i.e. do not daemonize)\n");
    fprintf(stderr, "-p, --port NUM\n");
    fprintf(stderr, "    use axiom raw port NUM for comunication (default: %d)\n",PORT);
    fprintf(stderr, "-n, --num_threads NUM\n");
    fprintf(stderr, "    number of threads (max: %d)\n", MAX_THREADS);
#ifndef NLOG
    fprintf(stderr, "-d, --debug\n");
    fprintf(stderr, "    override environment AXIOM_LOG_LEVEL settng it to DEBUG log level\n");
    fprintf(stderr, "    (note that this does not imply foreground run)\n");
#endif
    fprintf(stderr, "-V, --version\n");
    fprintf(stderr, "    print version\n");
}

/**
 * _usage with no message.
 */
#define usage() _usage(NULL)

static struct option long_options[] = {
    {"foreground", no_argument, 0, 'f'},
    {"port", required_argument, 0, 'p'},
    {"num_threads", required_argument, 0, 'n'},
    {"help", no_argument, 0, 'h'},
#ifndef NLOG
    {"debug", no_argument, 0, 'd'},
#endif
    {"version", no_argument, 0, 'V'},
    {0, 0, 0, 0}
};

#ifdef NLOG
static char const *options="p:n:hfV";
#else
static char const *options="p:n:hfdV";
#endif

void  *sender(void *d) {
    uint8_t buf[AXIOM_LONG_PAYLOAD_MAX_SIZE];
    int sz,ret,no;

    logmsg(LOG_INFO,"sender (eth -> axiom) starting");
    for (;;) {
        sz=read(tunh,buf,sizeof(buf));
        if (sz>0) {
            if (logmsg_is_enabled(LOG_DEBUG)) {
                if (sz>=12) {
                    logmsg(LOG_DEBUG,"eth recv: dmac=" MACSTR " smac=" MACSTR " sz=%d", MACVAL(buf), MACVAL(buf+ETH_ALEN), sz);
                } else {
                    logmsg(LOG_DEBUG,"eth recv: sz<12 error???");
                }
            }
            if (memcmp(buf,MAC,ETH_ALEN-1)==0) {
                ret=axiom_send(dev,buf[ETH_ALEN-1],PORT,sz,buf);
                if (!AXIOM_RET_IS_OK(ret)) {
                    elogmsg("axiom_send_raw()");
                }
            } else if (memcmp(buf,BROADCAST,ETH_ALEN)==0) {
                for (no=1; no<=num_nodes; no++) {
                    if (no==my_node) continue;
                    ret=axiom_send(dev,no,PORT,sz,buf);
                    if (!AXIOM_RET_IS_OK(ret)) {
                        elogmsg("axiom_send_raw()");
                    }
                }
            } else {
                logmsg(LOG_DEBUG,"discarded eth frame (bad destination mac)");
            }
        } else {
            logmsg(LOG_DEBUG,"eth recv: sz<=0 error???");
        }
    }

    logmsg(LOG_INFO,"sender end");
    return NULL;
}

void  *receiver(void *data) {

    axiom_node_id_t mit;
    axiom_port_t port;
    axiom_type_t type;

    uint8_t buf[AXIOM_LONG_PAYLOAD_MAX_SIZE];
    size_t sz,msz;
    int ret;

    logmsg(LOG_INFO,"receiver (axiom -> eht) starting");
    for (;;) {

        sz=sizeof(buf);
        ret=axiom_recv(dev,&mit,&port,&type,&sz,buf);
        if (!AXIOM_RET_IS_OK(ret)) {
            elogmsg("axiom_recv");
            continue;
        }
        if (sz<12) {
            logmsg(LOG_DEBUG,"ax  recv: sz<12 error???");
            continue;
        }
        logmsg(LOG_DEBUG, "ax  recv: dmac=" MACSTR " smac=" MACSTR " sz=%ld", MACVAL(buf), MACVAL(buf+ETH_ALEN), sz);

        if (memcmp(buf,mymac,ETH_ALEN)==0||memcmp(buf,BROADCAST,ETH_ALEN)==0) {
            msz=write(tunh,buf,sz);
            if (msz!=sz) {
                elogmsg("write()");
            }
        } else {
            logmsg(LOG_DEBUG,"discarded ax  packet (bad destination mac)");
        }
    }

    logmsg(LOG_INFO,"receiver end");
    return NULL;
}

/**
 * Allocate a tun/tap interface.
 * @param idx Interface number (i.e. 0 -> "ax0")
 * @param mac Last byte of mac address to set.
 * @return File handle or -1 on error.
 */
static int ethtap_alloc(int idx, int mac) {
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        elogmsg("open() tun/tap");
        return -1;
    }
        
    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     *
     * TAP -> ethernet
     * TUN -> ip
     */
    memset(&ifr, 0, sizeof (ifr));
#ifdef TUN_MODE
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
#else
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
#endif
    snprintf(ifr.ifr_name,IFNAMSIZ,"ax%d",idx);

    err = ioctl(fd, TUNSETIFF, (void *) &ifr);
    if (err< 0) {
        elogmsg("ioctl() TUNSETIF");
        close(fd);
        return -1;
    }

    logmsg(LOG_INFO,"interface name = %s",ifr.ifr_name);

    memset(&ifr, 0, sizeof (ifr));
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memcpy(ifr.ifr_hwaddr.sa_data,MAC,ETH_ALEN);
    ifr.ifr_hwaddr.sa_data[ETH_ALEN-1]=mac;
    memcpy(mymac,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
    
#if 0
    err = ioctl(fd, SIOCGIFHWADDR, (void *) &ifr);
    if (err< 0) {
        elogmsg("ioctl() SIOCGIFHWADDR (get MAC)");
        close(fd);
        return -1;
    }
    printf("MY ORIGINAL MAC IS = %02x:%02x:%02x:%02x:%02x:%02x",ifr.ifr_hwaddr.sa_data[0],ifr.ifr_hwaddr.sa_data[1],ifr.ifr_hwaddr.sa_data[2],ifr.ifr_hwaddr.sa_data[3],ifr.ifr_hwaddr.sa_data[4],ifr.ifr_hwaddr.sa_data[5]);
#endif
    
    err = ioctl(fd, SIOCSIFHWADDR, (void *) &ifr);
    if (err< 0) {
        elogmsg("ioctl() SIOCSIFHWADDR (set mac)");
        close(fd);
        return -1;
    }

    logmsg(LOG_INFO,"MAC address = " MACSTR, MACVAL(ifr.ifr_hwaddr.sa_data));
    
    return fd;
}

static void ethtap_prepare(int axiom_port) {
    axiom_err_t err;
    dev = axiom_open(NULL);
    if (dev == NULL) {
        elogmsg("axiom_open()");
        exit(EXIT_FAILURE);
    }
    num_nodes=axiom_get_num_nodes(dev);
    my_node=axiom_get_node_id(dev);
    err = axiom_bind(dev, axiom_port);
    if (!AXIOM_RET_IS_OK(err)) {
        elogmsg("axiom_bind()");
        exit(EXIT_FAILURE);
    }
    tunh=ethtap_alloc(0,(int) axiom_get_node_id(dev));
    if (tunh<=0) {
        exit(EXIT_FAILURE);
    }
}

/*
static void ethtap_deinit() {
    close(tunh);
    axiom_close(dev);
}
*/

int main(int argc, char **argv)
{
    pthread_t threcv[MAX_THREADS],thsend[MAX_THREADS];
    int num_threads = 1, i;
    int long_index;
    int err,res,opt;
    int foreground=0;

    logmsg_init();
    logmsg(LOG_INFO, "axiom-ethtap started");

    /* command line options */

    while ((opt = getopt_long(argc, argv, options, long_options, &long_index)) != -1) {
        switch (opt) {
            case 'f':
                foreground=1;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'n':
                num_threads = atoi(optarg);
                break;
#ifndef NLOG
            case 'd':
                logmsg_level=LOG_DEBUG;
                break;
#endif
            case 'V':
                printf("Version: %s\n", AXIOM_API_VERSION_STR);
                exit(EXIT_SUCCESS);
            case 'h':
                usage();
                exit(EXIT_SUCCESS);
            default:
                _usage("unknow option '%c'\n",opt);
                exit(EXIT_FAILURE);
        }

    }

    /* foreground or background */

    if (!foreground) {
        pid_t pid=daemonize(NULL,NULL,NULL,NULL,NULL,1,0,NULL);
        if (pid!=0) exit(EXIT_SUCCESS);
    }

    /* initialization */

    ethtap_prepare(port);

    logmsg(LOG_INFO,"axiom port = %d",PORT);

    /* threads */
    
    for (i = 0; i < num_threads; i++) {
        err = pthread_create(&threcv[i], NULL, receiver, NULL);
        if (err != 0) {
            elogmsg("pthread_create()");
            exit(EXIT_FAILURE);
        }

        err = pthread_create(&thsend[i], NULL, sender, NULL);
        if (err != 0) {
            elogmsg("pthread_create()");
            exit(EXIT_FAILURE);
        }
    }

    logmsg(LOG_DEBUG,"waiting...");

    for (i = 0; i < num_threads; i++) {
        res = pthread_join(thsend[i], NULL);
        if (res != 0) {
            elogmsg("pthread_join()");
            exit(EXIT_FAILURE);
        }

        res = pthread_join(threcv[i], NULL);
        if (res != 0) {
            elogmsg("pthread_join()");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
