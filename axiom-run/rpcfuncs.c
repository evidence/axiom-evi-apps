
#include "axiom_run_api.h"
#include "axiom-run.h"

extern axiom_err_t my_axiom_send_raw(axiom_dev_t *dev, axiom_port_t port, axiom_raw_payload_size_t size, axiom_raw_payload_t *payload);

int rpc_service(axiom_dev_t *dev, axiom_node_id_t src_node, int size, buffer_t *inmsg) {
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
                zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: axiom_send_raw() error %d while sending mesaage to node %d", msg, src_node);
            }
            break;
    }
    zlogmsg(LOG_WARN, LOGZ_MASTER, "MASTER: unknow CMD_RPC from node %d function 0x%02x", src_node, inmsg->header.rpc.function);
    return -1;
}