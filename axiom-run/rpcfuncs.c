#include <sys/uio.h>

#include "axiom_run_api.h"
#include "axiom-run.h"

#include "axiom_init_api.h"
#include "axiom_global_allocator.h"
#include "axiom_allocator_l2.h"

typedef struct rpc_allocator {
    enum {RA_INIT = 1, RA_ALLOC, RA_SETUP, RA_RELEASE} status;
    axiom_galloc_info_t info;
    axiom_allocator_l2_t l2_alloc;
    axiom_node_id_t master_node;
    header_t master_hdr_cache;
} rpc_allocator_t;

static rpc_allocator_t rpc_alloc = {
    .status = RA_RELEASE,
};

extern axiom_err_t my_axiom_send_raw(axiom_dev_t *dev, axiom_port_t port, axiom_raw_payload_size_t size, axiom_raw_payload_t *payload);

int rpc_init_allocator(axiom_app_id_t app_id) {
    if (rpc_alloc.status != RA_RELEASE) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: allocator status [%d] unexpected", rpc_alloc.status);
        return -1;
    }

    if (app_id == AXIOM_NULL_APP_ID) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: app_id not set [%d]", app_id);
        return -1;
    }

    rpc_alloc.info.app_id = app_id;
    rpc_alloc.master_node = AXIOM_NULL_NODE;
    rpc_alloc.status = RA_INIT;
    return 0;
}

void rpc_release_allocator(axiom_dev_t *dev) {
    axiom_err_t ret;

    if (rpc_alloc.status == RA_SETUP) {
        ret = axal12_release(dev, rpc_alloc.info.app_id);
        if (!AXIOM_RET_IS_OK(ret)) {
            zlogmsg(LOG_ERROR, LOGZ_MASTER,
                    "MASTER: axal12_release() error %d", ret);
        }

        axal_l2_release(&rpc_alloc.l2_alloc);

        rpc_alloc.status = RA_RELEASE;
    }
}

int rpc_setup_allocator(axiom_dev_t *dev, axiom_node_id_t src_node, size_t size, void *inmsg) {
    axiom_err_t ret;
    struct iovec send_iov[2];

    rpc_alloc.info.error = AXIOM_RET_OK;

    if (rpc_alloc.status != RA_ALLOC) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: allocator status [%d] unexpected", rpc_alloc.status);
        return -1;
    }

    ret = axal12_alloc_parsereply(inmsg, size, &rpc_alloc.info.private_start,
            &rpc_alloc.info.private_size, &rpc_alloc.info.shared_start,
            &rpc_alloc.info.shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: axal12_alloc_parsereply() error %d", ret);
        rpc_alloc.info.error = ret;
        goto reply;
    }

    /* setup the allocator */
    axal_l2_init(&rpc_alloc.l2_alloc, rpc_alloc.info.shared_start,
            rpc_alloc.info.shared_size);

    rpc_alloc.status = RA_SETUP;

    send_iov[0].iov_base = &rpc_alloc.master_hdr_cache;
    send_iov[0].iov_len = sizeof(rpc_alloc.master_hdr_cache);
    send_iov[1].iov_base = &rpc_alloc.info;
    send_iov[1].iov_len = sizeof(rpc_alloc.info);

reply:
    /* send back message to application master */
    ret = axiom_send_iov_raw(dev, rpc_alloc.master_node, slave_port,
            AXIOM_TYPE_RAW_DATA, send_iov[0].iov_len + send_iov[1].iov_len,
            send_iov, 2);
    if (!AXIOM_RET_IS_OK(ret)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw error %d", ret);
        return -1;
    }

    return 0;
}

static void axrun_rpc_alloc(axiom_dev_t *dev, axiom_node_id_t src_node, int size, buffer_t *inmsg) {
    axiom_galloc_info_t *info = (axiom_galloc_info_t *)inmsg->raw;
    axiom_err_t ret;

    info->error = AXIOM_RET_OK;

    if (rpc_alloc.status != RA_INIT) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: allocator status [%d] unexpected", rpc_alloc.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    rpc_alloc.master_node = src_node;
    rpc_alloc.master_hdr_cache = inmsg->header;

    rpc_alloc.status = RA_ALLOC;

    ret = axal12_alloc(dev, master_port, rpc_alloc.info.app_id,
            info->private_size, info->shared_size);
    if (!AXIOM_RET_IS_OK(ret)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axal12_alloc error %d", ret);
        rpc_alloc.status = RA_INIT;
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    return;
reply:

    /* send back the reply in case of error */
    ret = axiom_send_raw(dev, src_node, slave_port, AXIOM_TYPE_RAW_DATA, size,
            inmsg);
    if (!AXIOM_RET_IS_OK(ret)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw error %d", ret);
    }
}

static void axrun_rpc_get_prblock(axiom_dev_t *dev, axiom_node_id_t src_node, int size, buffer_t *inmsg) {
    axiom_galloc_info_t *info = (axiom_galloc_info_t *)inmsg->raw;
    axiom_err_t ret;

    info->error = AXIOM_RET_OK;

    if (rpc_alloc.status != RA_SETUP) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: allocator status [%d] unexpected", rpc_alloc.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    info->private_start = rpc_alloc.info.private_start;
    info->private_size = rpc_alloc.info.private_size;

reply:
    /* send back the reply */
    ret = axiom_send_raw(dev, src_node, slave_port, AXIOM_TYPE_RAW_DATA, size,
            inmsg);
    if (!AXIOM_RET_IS_OK(ret)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw error %d", ret);
        return;
    }
}

static void axrun_rpc_get_shblock(axiom_dev_t *dev, axiom_node_id_t src_node, int size, buffer_t *inmsg) {
    axiom_galloc_info_t *info = (axiom_galloc_info_t *)inmsg->raw;
    axiom_err_t ret;

    info->error = AXIOM_RET_OK;

    if (rpc_alloc.status != RA_SETUP) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: allocator status [%d] unexpected", rpc_alloc.status);
        info->error = AXIOM_RET_ERROR;
        goto reply;
    }

    /* alloc new blocks TODO: handle no memory error */
    ret = axal_l2_alloc(&rpc_alloc.l2_alloc, &(info->shared_start),
            &(info->shared_size), src_node);
    if (ret < 0) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER,
                "MASTER: axal_l2_alloc error %d", ret);
        info->error = AXIOM_RET_NOMEM;
        goto reply;
    }

reply:
    /* send back the reply */
    ret = axiom_send_raw(dev, src_node, slave_port, AXIOM_TYPE_RAW_DATA, size,
            inmsg);
    if (!AXIOM_RET_IS_OK(ret)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw error %d", ret);
        return;
    }
}

int rpc_service(axiom_dev_t *dev, axiom_node_id_t src_node, size_t size, buffer_t *inmsg) {
    axiom_msg_id_t msg;
    switch (inmsg->header.rpc.function) {
        case AXRUN_RPC_PING:
            //
            // RPC: ping request
            //
            // replay the same message to all slave
            //my_axiom_send_raw(dev, nodes, slave_port, size, (axiom_raw_payload_t*) inmsg);
            msg=axiom_send_raw(dev, src_node, slave_port,AXIOM_TYPE_RAW_DATA, size, inmsg);
            if (!AXIOM_RET_IS_OK(msg)) {
                zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw() error %d while sending message to node %d", msg, src_node);
            }
            break;
        case AXRUN_RPC_ALLOC:
            axrun_rpc_alloc(dev, src_node, size, inmsg);
            break;
        case AXRUN_RPC_GET_PRBLOCK:
            axrun_rpc_get_prblock(dev, src_node, size, inmsg);
            break;
        case AXRUN_RPC_GET_SHBLOCK:
            axrun_rpc_get_shblock(dev, src_node, size, inmsg);
            break;
        default:
            zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: unknow CMD_RPC from node %d function 0x%02x", src_node, inmsg->header.rpc.function);
            return -1;
    }

    return 0;
}
