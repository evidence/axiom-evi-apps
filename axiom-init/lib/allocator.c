#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "axiom_nic_raw_commands.h"
#include "axiom_nic_limits.h"
#include "axiom_nic_packets.h"
#include "axiom_nic_api_user.h"
#include "axiom_nic_init.h"
#include "axiom_init_api.h"


axiom_err_t
axinit_get_appid(axiom_dev_t *dev, axiom_port_t reply_port)
{
    axiom_allocator_payload_t alloc_payload;
    axiom_err_t ret;

    alloc_payload.command = AXIOM_CMD_ALLOC_APPID;
    alloc_payload.reply_port = reply_port;

    ret = axiom_send_raw(dev, AXIOM_MASTER_NODE_INIT, AXIOM_RAW_PORT_INIT,
            AXIOM_TYPE_RAW_DATA, sizeof(alloc_payload), &alloc_payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        return ret;
    }

    return AXIOM_RET_OK;
}

axiom_err_t
axinit_get_appid_reply(axiom_dev_t *dev, axiom_app_id_t *appid)
{
    axiom_allocator_payload_t payload;
    axiom_raw_payload_size_t size = sizeof(payload);
    axiom_node_id_t src;
    axiom_port_t port;
    axiom_type_t type;
    axiom_err_t ret;

    ret = axiom_recv_raw(dev, &src, &port, &type, &size, &payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        return ret;
    }

    if ((src != AXIOM_MASTER_NODE_INIT) ||
            (payload.command != AXIOM_CMD_ALLOC_APPID_REPLY) ||
            (payload.error != AXIOM_RET_OK)) {
        return AXIOM_RET_ERROR;
    }

    *appid = payload.app_id;

    return AXIOM_RET_OK;
}

axiom_err_t
axinit_alloc(axiom_dev_t *dev, axiom_port_t reply_port, axiom_app_id_t appid,
        uint64_t private_size, uint64_t shared_size)
{
    axiom_allocator_payload_t alloc_payload;
    axiom_err_t ret;

    alloc_payload.command = AXIOM_CMD_ALLOC;
    alloc_payload.reply_port = reply_port;
    alloc_payload.app_id = appid;
    alloc_payload.private_size = private_size;
    alloc_payload.shared_size = shared_size;

    ret = axiom_send_raw(dev, AXIOM_MASTER_NODE_INIT, AXIOM_RAW_PORT_INIT,
            AXIOM_TYPE_RAW_DATA, sizeof(alloc_payload), &alloc_payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        return ret;
    }

    return AXIOM_RET_OK;
}


axiom_err_t
axinit_release(axiom_dev_t *dev, axiom_port_t reply_port, axiom_app_id_t appid)
{
    axiom_allocator_payload_t alloc_payload;
    axiom_err_t ret;

    alloc_payload.command = AXIOM_CMD_ALLOC_RELEASE;
    alloc_payload.reply_port = reply_port;
    alloc_payload.app_id = appid;

    ret = axiom_send_raw(dev, AXIOM_MASTER_NODE_INIT, AXIOM_RAW_PORT_INIT,
            AXIOM_TYPE_RAW_DATA, sizeof(alloc_payload), &alloc_payload);
    if (!AXIOM_RET_IS_OK(ret)) {
        return ret;
    }

    return AXIOM_RET_OK;
}
