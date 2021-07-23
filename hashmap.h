#ifndef HASHMAP_H
#define HASHMAP_H

#include "common.h"

typedef struct HashMap HashMap;

HashMap *HashMap_create();

void HashMap_destroy(HashMap *hm);

void HashMap_insert(HashMap *map, char *key, u32 key_length, void *value);

void HashMap_delete(HashMap *map, char *key, u32 key_length);

void *HashMap_get(HashMap *map, char *key, u32 key_length);

#endif
