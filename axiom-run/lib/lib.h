
#ifndef LIB_H
#define LIB_H

#include <sys/syscall.h>
#include <stdint.h>

static inline  uint64_t gettid() {
    return syscall(SYS_gettid);
}

#endif
