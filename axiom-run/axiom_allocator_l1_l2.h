/*!
 * \file axiom_allocator_l1_l2.h
 *
 * \version     v0.8
 * \date        2016-09-23
 *
 * This file contains the AXIOM allocator interface between level 1 and level 2
 */
#ifndef AXIOM_ALLOCATOR_L1_L2_h
#define AXIOM_ALLOCATOR_L1_L2_h

    /*
     * \brief Request an unique application ID to the AXIOM_MASTER_NODE_INIT
     *
     * \param dev           Axiom device private data pointer
     * \param reply_port    Port where you want to receive the reply
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
axiom_err_t
axal12_get_appid(axiom_dev_t *dev, axiom_port_t reply_port);

    /*
     * \brief Waiting for a reply from AXIOM_MASTER_NODE_INIT with an
     *        unique application ID
     *
     * \param dev           Axiom device private data pointer
     * \param[out] appid    Application ID received for the application
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
axiom_err_t
axal12_get_appid_reply(axiom_dev_t *dev, axiom_app_id_t *appid);


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
axiom_err_t
axal12_alloc(axiom_dev_t *dev, axiom_port_t reply_port, axiom_app_id_t appid,
        uint64_t private_size, uint64_t shared_size);


axiom_err_t
axal12_alloc_parsereply(void *payload, size_t payload_size,
        uint64_t *private_start, uint64_t *private_size, uint64_t *shared_start,
        uint64_t *shared_size);

    /*
     * \brief Request to the AXIOM_MASTER_NODE_INIT to release the application
     *        ID and the private and shared regions.
     *
     * \param dev               Axiom device private data pointer
     * \param appid             Application ID of the application
     *
     * \return Returns AXIOM_RET_OK on success, an error otherwise.
     */
axiom_err_t
axal12_release(axiom_dev_t *dev, axiom_app_id_t appid);
#endif /* AXIOM_ALLOCATOR_L1_L2_h */
