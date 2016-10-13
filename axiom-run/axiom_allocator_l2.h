/*!
 * \file axiom_allocator_l2.h
 *
 * \version     v0.8
 * \date        2016-09-23
 *
 * This file contains the AXIOM allocator level 2
 */
#ifndef AXIOM_ALLOCATOR_L2_h
#define AXIOM_ALLOCATOR_L2_h

#include "axiom_global_allocator.h"

int
axiom_allocator_l2_init(axiom_app_id_t app_id);

void
axiom_allocator_l2_release(axiom_dev_t *dev);

int
axiom_allocator_l2_alloc_reply(axiom_dev_t *dev, axiom_node_id_t src_node,
        size_t size, void *inmsg, axiom_galloc_info_t *info);

int
axiom_allocator_l2_alloc(axiom_dev_t *dev, axiom_node_id_t src_node,
        axiom_port_t master_port, size_t size, void *buffer);
int
axiom_allocator_l2_get_prblock(axiom_dev_t *dev, axiom_node_id_t src_node,
        size_t size, void *buffer);
int
axiom_allocator_l2_get_shblock(axiom_dev_t *dev, axiom_node_id_t src_node,
        size_t size, void *buffer);
#endif /* AXIOM_ALLOCATOR_L2_h */
