/*!
 * \file test.h
 *
 * \version     v0.10
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/*
 * random numbers
 */

typedef struct {
  int counter;
  int value;
  unsigned int state;
} rand_t;

static inline void rand_init(rand_t *p, unsigned int seed) {
    p->counter=0;
    p->value=0;
    p->state=seed;
}

static inline uint8_t rand_next(rand_t *p) {
    return p->counter--==0?(p->counter=sizeof(int)-1,p->value=rand_r(&p->state),p->value&0xff):(p->value>>=8,p->value&0xff);
}

static inline unsigned int rand_next_unsigned(rand_t *p) {
    unsigned int res=0;
    int c=sizeof(unsigned int);
    while (c-->0) { res<<=8; res|=rand_next(p); }
    return res;
}

/*
 * misc functions
 */

static inline int asc2int(const char *s) {
    return (int)strtol(s,NULL,0);
}

extern void dump(FILE *fout, uint8_t *addr, size_t size);

/*
 * printf defines
 */

#define MEMBLOCKSTR "%p...%p"
#define MEMBLOCKDATA(addr,size) ((uint8_t*)(addr)),((uint8_t*)(addr))+((size_t)(size))-1

#define MEMBLOCKFULLSTR "%p...%p %lu 0x%lx (%lu KiB %lu MiB)"
#define MEMBLOCKFULLDATA(addr,size) ((uint8_t*)(addr)),((uint8_t*)(addr))+((size_t)(size))-1,((size_t)(size)),((size_t)(size)),((size_t)(size))/1024,((size_t)(size))/1024/1024

#define SIZESTR "%lu 0x%lx (%lu KiB %lu MiB)"
#define SIZEDATA(size) ((size_t)(size)),((size_t)(size)),((size_t)(size))/1024,((size_t)(size))/1024/1024

#endif /* TEST_H */

