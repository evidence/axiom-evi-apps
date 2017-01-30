/*!
 * \file strlist.c
 *
 * \version     v0.11
 *
 * String list manipulation functions.
 *
 * Copyright (C) 2016, Evidence Srl.
 * Terms of use are as specified in COPYING
 */
#include <stdlib.h>
#include <string.h>

#include "axiom_common.h"

/**
 * Insert a string into the string list.
 * @param p The string list.
 * @param s The string to insert.
 * @param head true insert into head else into tail.
 */
static void _insert(strlist_t *p, char *s, int head) {
    void *ptr;
    int idx;
    if (p->size == 0) {
        ptr = malloc(sizeof (char*));
    } else {
        ptr = realloc(p->data, sizeof (char*)*(p->size + 1));
    }
    if (ptr == NULL) {
        elogmsg("realloc() failure");
        return;
    }
    p->data = (char**) ptr;
    p->size++;
    if (head) {
        memmove(p->data, p->data + 1, p->size - 1);
        idx = 0;
    } else {
        idx = p->size - 1;
    }
    if (s == NULL) {
        p->data[idx] = NULL;
    } else {
        int size = strlen(s) + 1;
        char *s2 = malloc(size);
        if (s2 == NULL) {
            elogmsg("malloc() failure");
            return;
        }
        strlcpy(s2, s, size);
        p->data[idx] = s2;
    }
}

/* see axiom_common.h */
void sl_append(strlist_t *p, char *s) {
    _insert(p, s, 0);
}

/* see axiom_common.h */
void sl_insert(strlist_t *p, char *s) {
    _insert(p, s, 1);
}

/* see axiom_common.h */
char** sl_get(strlist_t *p) {
    return p->data;
}

/* see axiom_common.h */
void sl_init(strlist_t *p) {
    p->size = 0;
    p->data = NULL;
}

/**
 * Count a NULL terminated array of string.
 * @param args The array of string.
 * @return The number of elements of array.
 */
static inline int count(char **args) {
    register int c = 0;
    if (args == NULL) return 0;
    while (*args++ != NULL) c++;
    return c;
}

/**
 * Insert the array of NULL terminated string to the list.
 * @param p The list.
 * @param args The array of string.
 * @param head true insert into head else into tail
 */
static inline void _insert_all(strlist_t *p, char **args, int head) {
    // slow implementation :-(
    if (args == NULL) return;
    while (*args != NULL) {
        _insert(p, *args, head);
        args++;
    }
}

/* see axiom_common.h */
void sl_append_all(strlist_t *p, char **args) {
    _insert_all(p, args, 0);
}

/* see axiom_common.h */
void sl_insert_all(strlist_t *p, char **args) {
    _insert_all(p, args, 1);
}

/* see axiom_common.h */
void sl_free(strlist_t *p) {
    if (p->size > 0) {
        int i;
        for (i = 0; i < p->size; i++) {
            if (p->data[i] != NULL) free(p->data[i]);
        }
        free(p->data);
    }
    sl_init(p);
}
