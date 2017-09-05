/*!
 * \file axiom-netperf.h
 *
 * \version     v0.13
 * \date        2017-09-05
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_NETPERF_h
#define AXIOM_NETPERF_h

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


/*! \brief Message payload for the axiom-netperf application */
typedef struct axiom_netperf_payload {
    uint8_t  command;           /*!< \brief Command of netperf messages */
    uint8_t  padding[7];
    uint64_t total_bytes;       /*!< \brief Total bytes of the stream */
    uint64_t elapsed_time;      /*!< \brief Time elapsed to receive data */
    uint8_t  type;              /*!< \brief Type of message used in the test */
    uint8_t  magic;             /*!< \brief Magic byte written in the payload */
    uint8_t  error;             /*!< \brief Error report */
    uint8_t  reply_port;
    uint8_t  spare[100];
} axiom_netperf_payload_t;

typedef struct {

} axnetperf_client_t;

typedef struct {
    axiom_dev_t *dev;
    axiom_netperf_type_t np_type;
    axiom_node_id_t dest_node;
    axiom_port_t server_port;
    axiom_port_t client_port;
    struct timespec start_ts;
    struct timespec end_ts;
    size_t  payload_size;
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t sent_bytes;
    uint64_t sent_raw_bytes;
    void *rdma_zone;
    uint64_t rdma_size;
    uint8_t magic;
    int rdma_sync;

} axnetperf_status_t;

axiom_err_t axnetperf_client(axnetperf_status_t *s);
axiom_err_t axnetperf_server(axnetperf_status_t *s);

#endif /* !AXIOM_NETPERF_h */
