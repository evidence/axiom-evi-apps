/*!
 * \file axiom_run_api.h
 *
 * \version     v1.0
 *
 * axiom-run services.
 * API for applications using service provided by axiom-run application.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef AXIOM_RUN_API_H
#define AXIOM_RUN_API_H

#ifdef __cplusplus
extern "C" {
#endif

    /** Maximun barrier identification allowed. */
#define AXRUN_MAX_BARRIER_ID 15

    /**
     * Synchronization on a barrier.
     * Used to sychronize all the applications started by an axiom-run instance.
     * @param barrier_id the barrier identification.
     * @param verbose if true in case of error emit verbose messages on stderr
     * @return 0 on success -1 on error (setting errno)
     */
    int axrun_sync(unsigned barrier_id, int verbose);

#define AXRUN_RPC_PING                  0
#define AXRUN_RPC_ALLOC                 1
#define AXRUN_RPC_GET_REGIONS           2
#define AXRUN_RPC_ALLOC_SHBLOCK         3

    int axrun_rpc(int function, size_t send_size, void *send_payload,
            size_t *recv_size, void *recv_payload, int verbose);

    /**
     *  @brief  This function returns the bitmask of nodes involved in the
     *          execution of the application.
     *
     *  @return Returns 64-bit mask
     */
    uint64_t axrun_get_nodes(void);

    /**
     *  @brief  This function returns the number of nodes involved in the
     *          execution of the application.
     *
     *  @return Returns a positive number on success, -1 on error.
     */
    int axrun_get_num_nodes(void);

#ifdef __cplusplus
}
#endif

#endif /* AXIOM_RUN_API_H */
