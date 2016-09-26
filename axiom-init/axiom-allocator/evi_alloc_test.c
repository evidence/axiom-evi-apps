#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "evi_alloc.h"

int main()
{
    evi_alloc_t ea;
    int ret;

    ret = evia_init(&ea, 128);
    printf("ret %d\n", ret);
    evia_dump(&ea);

    ret = evia_alloc(&ea, 33, 4);
    printf("ret %d\n", ret);
    evia_dump(&ea);

    ret = evia_alloc(&ea, 66, 10);
    printf("ret %d\n", ret);
    evia_dump(&ea);

    evia_free(&ea, 66);
    evia_dump(&ea);

    ret = evia_alloc(&ea, 11, 100);
    printf("ret %d\n", ret);
    evia_dump(&ea);

    ret = evia_alloc(&ea, 77, 40);
    printf("ret %d\n", ret);
    evia_dump(&ea);

    return 0;
}
