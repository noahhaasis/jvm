#ifndef HASHMAP_H
#define HASHMAP_H

#include "common.h"
#include "string.h"

typedef struct HashMap HashMap;

HashMap *HashMap_create();

void HashMap_destroy(HashMap *hm);

void HashMap_insert(HashMap *map, String key, void *value);

void HashMap_delete(HashMap *map, String key);

void *HashMap_get(HashMap *map, String key);

#endif
