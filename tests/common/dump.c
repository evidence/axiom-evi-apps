/*!
 * \file dump.c
 *
 * \version     v0.13
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void dump(FILE *fout, uint8_t *addr, size_t size) {
    char buffer[96];
    int idx=0;
    int c=0;
    while (size-->0) {
        if (c==0) {
            if (idx>0) fprintf(fout,"%s\n",buffer);
            idx=0;
            idx=sprintf(buffer,"%p: ",addr);
            c=16;
        }
        idx+=sprintf(buffer+idx, "0x%02x ",*addr);
        addr++;
        c--;
    }
    if (idx>0) {
        fprintf(fout,"%s\n",buffer);
    }
}
