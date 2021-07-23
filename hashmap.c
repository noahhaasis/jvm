#include "hashmap.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NUM_BUCKETS 64

typedef uint64_t u64;

typedef struct bucket_item bucket_item;
struct bucket_item {
  u64 hash;
  char *key;
  void *value;
  bucket_item *next;
};

// TODO(noah):
// Use a stretchy buffer instead of a linked list?
// And then maybe split the hashes into a seperate buffer (data-oriented design)

struct hashmap {
  // Each bucket is a linked list of (u64 hash, char *key, void *value)
  // where the keys all have the same hash
  bucket_item *buckets[NUM_BUCKETS];
};

u64 hash(char *str) { // TODO
  u64 res = 0;
  for (char c = *str; c != '\0'; c = *(++str)) {
    res += c;
  }
  return res;
}

hashmap *create_hashmap() {
  return calloc(sizeof(hashmap), 1);
}

void destroy_hashmap(hashmap *hm) {
  for (int i = 0; i < NUM_BUCKETS; i++) {
    // TODO
  }
  free(hm);
}

void hm_insert(hashmap *map, char *key, void *value) {
  assert(map);

  u64 h = hash(key);
  u64 index = h % NUM_BUCKETS;
  bucket_item *current_item = map->buckets[index];
  while (current_item->next != NULL) {
    if (current_item->hash == h && strcmp(key, current_item->key) == 0) {
      free(current_item->value);
      current_item->value = value;
      return; // Key already exists
    }
    current_item = current_item->next;
  }

  if (current_item->hash == h && strcmp(key, current_item->key) == 0) {
    free(current_item->value);
    current_item->value = value;
    return; // Key already exists
  }

  current_item->next = malloc(sizeof(bucket_item));
  current_item->next->hash = h;
  current_item->next->key = key; // TODO: Copy?
  current_item->next->value = value;
  current_item->next->next = NULL;
}

void hm_delete(hashmap *map, char *key) {
  assert(map);

  u64 h = hash(key);
  u64 index = h % NUM_BUCKETS;

  bucket_item *current_item = map->buckets[index];
  bucket_item *prev_item = NULL;
  while (current_item != NULL) {
    if (current_item->hash == h && strcmp(key, current_item->key) == 0) {
      if (prev_item == NULL) {
        map->buckets[index] = current_item->next;
      } else {
        prev_item->next = current_item->next;
        free(current_item->value);
        free(current_item);
      }
      return;
    }
    
    prev_item = current_item;
    current_item = current_item->next;
  }
}

void *hm_get(hashmap *map, char *key) {
  assert(map);
  u64 h = hash(key);
  u64 index = h % NUM_BUCKETS;

  bucket_item *current_item = map->buckets[index];
  for (; current_item != NULL; current_item = current_item->next) {
    if (current_item->hash == h && strcmp(key, current_item->key) == 0) {
      return current_item->value;
    }
  }

  return NULL;
}


