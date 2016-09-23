#ifndef EVI_ALLOC_h
#define EVI_ALLOC_h
/*!
 * \file evi_alloc.h
 *
 * \version     v0.8
 * \date        2016-05-03
 *
 * This file contains the EVI alloc manager.
 */

#define EVIA_NONE       (evia_elem_t)(-1)       /*!< \brief none elements */

#ifdef __KERNEL__
#define EVIA_MALLOC(_1)  (kmalloc(_1, GFP_KERNEL))
#define EVIA_FREE(_1)    (kfree(_1))
#define EVIA_PRINTF(...)  (printk(KERN_INFO __VA_ARGS__))
#else /* !__KERNEL__ */
#define EVIA_MALLOC(_1)  (malloc(_1))
#define EVIA_FREE(_1)    (free(_1))
#define EVIA_PRINTF(...)  (printf(__VA_ARGS__))
#endif /* __KERNEL__ */

/*! \brief Element used in the EVI alloc manager */
typedef uint8_t evia_elem_t;

/*! \brief EVI alloc status */
typedef struct evi_alloc {
    int elems;                  /*!< number of total elements */
    int free;                   /*!< number of free elements */
    evia_elem_t *array;         /*!< \brief array of elements */
} evi_alloc_t;

/*!
 * \brief Release the resorces for the EVI alloc status
 *
 * \param ea             EVI alloc status pointer
 */
static void
evia_release(evi_alloc_t *ea)
{
    if (ea->array)
        EVIA_FREE(ea->array);

    ea->array = NULL;
}

/*!
 * \brief Init the EVI alloc status
 *
 * \param ea            EVI alloc status pointer
 * \param elems         Number of initial free elements
 *
 * \return 0 on success, otherwise -1
 */
static int
evia_init(evi_alloc_t *ea, int elems)
{
    int i;

    ea->array = EVIA_MALLOC(elems * sizeof(*(ea->array)));
    if (ea->array == NULL)
        goto err;

    ea->elems = elems;
    ea->free = elems;

    //memset(ea->array, EVIA_NONE, elems * sizeof(*(ea->array)));
    for (i = 0; i < ea->elems; i++) {
        ea->array[i] = EVIA_NONE;
    }

    return 0;

err:
    evia_release(ea);
    return -1;
}

static int
evia_alloc(evi_alloc_t *ea, evia_elem_t value, int num)
{
    int i, count = 0, start;

    if (num > ea->free) {
        return -1;
    }

    for (i = 0; (count < num) && (i < ea->elems); i++) {
        if (ea->array[i] == EVIA_NONE) {
            count++;
        } else {
            count = 0;
        }
    }

    if (count != num) {
        return -1;
    }

    start = i - count;

    for (i = start; i < start + count; i++) {
        ea->array[i] = value;
    }

    ea->free -= count;

    return start;
}

static void
evia_free(evi_alloc_t *ea, evia_elem_t value)
{
    int i;

    for (i = 0; i < ea->elems; i++) {
        if (ea->array[i] == value) {
            ea->array[i] = EVIA_NONE;
            ea->free++;
        }
    }
}

static void
evia_dump(evi_alloc_t *ea)
{
    int i;

    EVIA_PRINTF("evi_alloc: free %d - total %d", ea->free, ea->elems);
    for (i = 0; i < ea->elems; i++) {
        if (!(i % 10))
            EVIA_PRINTF("\n %04d:  ", i);

        EVIA_PRINTF("0x%02x ", ea->array[i]);
    }
    EVIA_PRINTF("\n");
}
#endif /* EVI_ALLOC_h */
