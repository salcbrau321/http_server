#ifndef HEADER_MAP_H
#define HEADER_MAP_H

#include <stdlib.h>

typedef struct {
    char* name;
    char* value;
    struct Header* next;
} Header;

typedef struct {
    Header **items;
    size_t count;
} HeaderMap;

HeaderMap* header_map_new();
void header_map_add(HeaderMap* map, const char* name, const char* value);
const char *header_map_get(const HeaderMap *map, const char *name);
void header_map_free(HeaderMap* map);

#endif
