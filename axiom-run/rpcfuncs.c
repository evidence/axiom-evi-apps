#include <sys/uio.h>

#include "axiom_run_api.h"
#include "axiom-run.h"

#include "axiom_init_api.h"
#include "axiom_nic_raw_commands.h"
#include "axiom_allocator_l2.h"

typedef struct {
    axiom_node_id_t reply_node;
    axiom_port_t reply_port;
    header_t reply_hdr;
} rpc_reply_t;
/* TODO: now we support only one pending reply */
static rpc_reply_t rpc_reply;


extern axiom_err_t my_axiom_send_raw(axiom_dev_t *dev, axiom_port_t port, axiom_raw_payload_size_t size, axiom_raw_payload_t *payload);

int rpc_init(axiom_app_id_t app_id) {
    return axiom_al2_init(app_id);
}

void rpc_release(axiom_dev_t *dev) {
    axiom_al2_release(dev);
}

void rpc_postpone_reply(axiom_node_id_t reply_node, axiom_port_t reply_port,
        header_t reply_hdr)
{
    rpc_reply.reply_node = reply_node;
    rpc_reply.reply_port = reply_port;
    rpc_reply.reply_hdr = reply_hdr;
}

int rpc_send_reply(axiom_dev_t *dev, size_t size, void *buffer)
{
    struct iovec send_iov[2];
    axiom_err_t err;

    send_iov[0].iov_base = &rpc_reply.reply_hdr;
    send_iov[0].iov_len = sizeof(rpc_reply.reply_hdr);
    send_iov[1].iov_base = buffer;
    send_iov[1].iov_len = size;

    /* send back message to application master */
    err = axiom_send_iov_raw(dev, rpc_reply.reply_node,
            rpc_reply.reply_port, AXIOM_TYPE_RAW_DATA,
            send_iov[0].iov_len + send_iov[1].iov_len, send_iov, 2);
    if (!AXIOM_RET_IS_OK(err)) {
        zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw error %d", err);
        return -1;
    }

    return 0;
}

int rpc_service(axiom_dev_t *dev, axiom_node_id_t src_node, size_t size, buffer_t *inmsg) {
    axiom_err_t err;
    int reply = 0;

    /*
     * Reply received from AXIOM ALLOCATOR MASTER INIT, propagated to master
     * application (axiom-run slave on master node)
     */
    if (inmsg->header.command == AXIOM_CMD_ALLOC_REPLY) {
        axiom_alloc_msg_t info;
        int ret;

        ret = axiom_al2_alloc_reply(dev, size, inmsg, &info);
        if (ret) {
            /* now we can send the reply of AXRUN_RPC_ALLOC previously received */
            return rpc_send_reply(dev, sizeof(info), &info);
        }

        return 0;
    }

    switch (inmsg->header.rpc.function) {
        case AXRUN_RPC_PING:
            //
            // RPC: ping request
            //
            // replay the same message to all slave
            //my_axiom_send_raw(dev, nodes, slave_port, size, (axiom_raw_payload_t*) inmsg);
            reply = 1;
            break;
        case AXRUN_RPC_ALLOC:
            reply = axiom_al2_alloc(dev, master_port, size - sizeof(inmsg->header), &inmsg->raw);
            if (!reply) {
                /* postpone reply to slave, because we are waiting the reply from MASTER INIT */
                rpc_postpone_reply(src_node, slave_port, inmsg->header);
            }
            break;
        case AXRUN_RPC_GET_REGIONS:
            reply = axiom_al2_get_regions(dev, src_node, size - sizeof(inmsg->header), &inmsg->raw);
            break;
        case AXRUN_RPC_ALLOC_SHBLOCK:
            reply = axiom_al2_alloc_shblock(dev, src_node, size - sizeof(inmsg->header), &inmsg->raw);
            break;
        default:
            zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: unknow CMD_RPC from node "
                    "%d function 0x%02x", src_node, inmsg->header.rpc.function);
            return -1;
    }

    if (reply) {
        err = axiom_send_raw(dev, src_node, slave_port, AXIOM_TYPE_RAW_DATA,
                size, inmsg);
        if (!AXIOM_RET_IS_OK(err)) {
            zlogmsg(LOG_ERROR, LOGZ_MASTER, "MASTER: axiom_send_raw() error %d "
                    "while sending message to node %d", err, src_node);
        }
    }

    return 0;
}
