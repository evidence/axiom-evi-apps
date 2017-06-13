/*!
 * \file axiom_init_api.h
 *
 * \version     v0.13
 *
 * axiom-init API services.
 * API for applications using service provided by axiom-init application.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
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

#ifdef __cplusplus
}
#endif

#endif

