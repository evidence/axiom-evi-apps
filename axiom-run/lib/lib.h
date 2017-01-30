/*!
 * \file lib.h
 *
 * \version     v0.11
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef LIB_H
#define LIB_H

#include <sys/syscall.h>
#include <stdint.h>

static inline  uint64_t gettid() {
    return syscall(SYS_gettid);
}

#endif
