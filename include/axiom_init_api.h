/**
 * axiom-init API services.
 * API for applications using service provided by axiom-init application.
 *
 * @file axiom_init_api.h
 * @version 0.7
 */
#ifndef AXIOM_INIT_API_H
#define AXIOM_INIT_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "axiom_nic_types.h"

    /** Flag for axinit_execvpe: contact all axiom-init. */
#define AXINIT_EXEC_BROADCAST 0x01
    /** Flag for axinit_execvpe: contact all axiom-init but not the self node axiom-init. */
#define AXINIT_EXEC_NOSELF    0x02

    /**
     * Exec an application on a node.
     * The semantic of the filename, argv, envp parameters are the same of the system execvpe function.
     *
     * @param dev Device used for the comunication.
     * @param port Port of the axiom-init (default AXIOM_RAW_PORT_INIT)
     * @param node Node where to run the applicartion.
     * @param flags AXINIT_EXEC_? flags.
     * @param filename Executable.
     * @param argv Arguments to the executable.
     * @param envp Environment of the executable.
     * @return AXIOM_RET_OK on success else on error.
     */
    int axinit_execvpe(axiom_dev_t *dev, axiom_port_t port, axiom_node_id_t node, int flags, const char *filename, char *const argv[], char *const envp[]);

    /*
     * \brief Request an unique application ID to the AXIOM_MASTER_NODE_INIT
     *
     * \param dev           Axiom device private data pointer
     * \param reply_port    Port where you want to receive the reply
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
    axiom_err_t axinit_get_appid(axiom_dev_t *dev, axiom_port_t reply_port);

    /*
     * \brief Request an unique application ID to the AXIOM_MASTER_NODE_INIT
     *
     * \param dev           Axiom device private data pointer
     * \param node          Destination node id (default AXIOM_MASTER_NODE_INIT)
     * \param[out] appid    Application ID received for the application
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
    axiom_err_t axinit_get_appid_reply(axiom_dev_t *dev, axiom_app_id_t *appid);

    /*
     * \brief Request to the AXIOM_MASTER_NODE_INIT to allocate private and
     *        shared regions for the specified application (app_id)
     *
     * \param dev               Axiom device private data pointer
     * \param reply_port        Port where you want to receive the reply
     * \param appid             Application ID of the application
     * \param private_size      Size of private region (return size block
     *                          aligned)
     * \param shared_size       Size of private region (return size block
     *                          aligned)
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
    axiom_err_t axinit_alloc(axiom_dev_t *dev, axiom_port_t reply_port,
            axiom_app_id_t appid, uint64_t private_size, uint64_t shared_size);

    /*
     * \brief Request to the AXIOM_MASTER_NODE_INIT to release the application
     *        ID and the private and shared regions.
     *
     * \param dev               Axiom device private data pointer
     * \param reply_port        Port where you want to receive the reply
     * \param appid             Application ID of the application
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
    axiom_err_t axinit_release(axiom_dev_t *dev, axiom_port_t reply_port,
            axiom_app_id_t appid);

#ifdef __cplusplus
}
#endif

#endif

