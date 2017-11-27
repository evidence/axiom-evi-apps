/*!
 * \file axiom-netperf.h
 *
 * \version     v0.15
 * \date        2017-09-05
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_NETPERF_h
#define AXIOM_NETPERF_h
#include <sys/syscall.h>

#define AXIOM_NETPERF_DEF_CHAR_SCALE    'B'
#define AXIOM_NETPERF_DEF_DATA_SCALE    10
#define AXIOM_NETPERF_DEF_DATA_LENGTH   1024
#define AXIOM_NETPERF_DEF_RDMA_PSIZE    AXIOM_RDMA_PAYLOAD_MAX_SIZE
#define AXIOM_NETPERF_DEF_RAW_PSIZE     AXIOM_RAW_PAYLOAD_MAX_SIZE
#define AXIOM_NETPERF_DEF_LONG_PSIZE    AXIOM_LONG_PAYLOAD_MAX_SIZE
#define AXIOM_NETPERF_DEF_TYPE          AXNP_LONG
#define AXIOM_NETPERF_DEF_PORT          3

#define AXNP_RES_BYTE_SCALE             1024 / 1024 / 1024
#define AXNP_RES_PKT_SCALE              1000
#define AXNP_MAX_THREADS                64

typedef enum {
    AXN_STATE_NULL,
    AXN_STATE_INIT,
    AXN_STATE_START,
    AXN_STATE_END,
    AXN_STATE_STOP,
    AXN_STATE_ERROR
} axnetperf_state_t;

/*! \brief Message payload for the axiom-netperf application */
typedef struct axiom_netperf_payload {
    uint8_t  command;           /*!< \brief Command of netperf messages */
    uint8_t  padding[7];
    uint64_t total_bytes;       /*!< \brief Total bytes of the stream */
    uint64_t elapsed_time;      /*!< \brief Time elapsed to receive data */
    uint8_t  type;              /*!< \brief Type of message used in the test */
    uint64_t magic;            /*!< \brief Magic byte written in the payload */
    uint8_t  error;             /*!< \brief Error report */
    uint8_t  reply_port;
    uint8_t  spare[93];
} axiom_netperf_payload_t;

typedef struct {
    struct timespec start_ts;
    struct timespec end_ts;
    size_t  payload_size;
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t sent_bytes;
    uint64_t sent_raw_bytes;
    void *rdma_zone;
    uint64_t rdma_size;
    uint64_t magic;
    int rdma_sync;
} axnetperf_client_t;

typedef struct {
    struct timespec start_ts;   /*!< \brief timestamp of the first byte */
    struct timespec cur_ts;     /*!< \brief current timestamp */
    uint64_t expected_bytes;    /*!< \brief total bytes that will be received */
    uint64_t received_bytes;    /*!< \brief number of bytes received */
    uint8_t reply_port;
} axnetperf_server_t;

typedef struct {
    axiom_dev_t *dev;
    axnetperf_state_t state;
    axiom_netperf_type_t np_type;
    axiom_node_id_t server_id;
    axiom_port_t client_port;
    axiom_port_t server_port;
    unsigned int num_threads;
    pthread_t threads[AXNP_MAX_THREADS];
    pthread_mutex_t mutex;
    axnetperf_client_t client;
    axnetperf_server_t server[AXIOM_NODES_NUM];
} axnetperf_status_t;

void *axnetperf_client(void *s);
void *axnetperf_server(void *s);

static inline long
gettid(void)
{
    return syscall(SYS_gettid);
}

#endif /* !AXIOM_NETPERF_h */
