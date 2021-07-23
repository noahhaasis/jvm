#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct hashmap hashmap;

hashmap *create_hashmap();

void destroy_hashmap(hashmap *hm);

void hm_insert(hashmap *map, char *key, void *value);

void hm_delete(hashmap *map, char *key);

void *hm_get(hashmap *map, char *key);

#endif
