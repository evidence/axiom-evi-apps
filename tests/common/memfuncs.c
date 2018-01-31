/*!
 * \file memfuncs.c
 *
 * \version     v1.0
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* rationale: because the rdma memory is not cached and so must be accessed with a 64bits alignment! */

void *mymemset(void *s, int c, size_t n) {
    uint64_t *p=(uint64_t*)s;
    uint64_t myc=c&0xff;
    uint64_t v=0;
    int i=0;
    /* paranoia checks... */
    if (((uint64_t)s)%8!=0) {
        fprintf(stderr,"mymemset: src must be aligned by 8");
        abort();
    }
    if (n%8!=0) {
        fprintf(stderr,"mymemset: n must be multiple of 8");
        abort();
    }
    /* do all... */
    for (i=0;i<8;i++) { v|=myc; myc<<=8; }
    n/=8;
    for (i=0;i<n;i++) *(p+i)=v;
    return s;
}
