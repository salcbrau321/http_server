#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "header_map.h"

static unsigned fnv1a(const char* s) {
    unsigned h = 2166136261u;
    for (; *s; ++s) {
        h ^= (unsigned char)tolower((unsigned char)*s);
        h *= 16777619u;
    }
    return h;
}

HeaderMap* header_map_new() {
    HeaderMap *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->count = 32;
    m->items = calloc(m->count, sizeof *m->items);
    if (!m->items) {
        free(m);
        return NULL;
    }
    return m;
}

void header_map_add(HeaderMap *m, const char* name, const char* value) {
    unsigned idx = fnv1a(name) & (m->count - 1);
    Header **pp = &m->items[idx];

    for (Header *h = *pp; h; h = h->next) {
        if (strcasecmp(h->name, name) == 0) {
            free(h->value);
            h->value = strdup(value);
            return;
        }
    }

    Header* h = malloc(sizeof *h);
    h->name = strdup(name);
    h->value = strdup(value);
    h->next = *pp;
    *pp = h;
}

const char *header_map_get(const HeaderMap *m, const char *name) {
    unsigned h = fnv1a(name) & (m->count - 1);
    for (Header *e = m->items[h]; e; e = e->next) {
        if (strcasecmp(e->name, name) == 0) {
            return e->value;
        }
    }
    return NULL;
}

void header_map_free(HeaderMap *m) {
    for (size_t i = 0; i < m->count; i++) {
        Header *e = m->items[i];
        while (e) {
            Header *next = e->next;
            free(e->name);
            free(e->value);
            free(e);
            e = next;
        }
    }
    free(m->items);
    free(m);
}
