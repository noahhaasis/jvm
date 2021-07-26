#include "hashmap.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define NUM_BUCKETS 64

typedef struct BucketItem BucketItem;
struct BucketItem {
  u64 hash;
  char *key;
  u32 key_length;
  void *value;
  BucketItem *next;
};

// TODO(noah):
// Use a stretchy buffer instead of a linked list?
// And then maybe split the hashes into a seperate buffer (data-oriented design)

struct HashMap {
  // Each bucket is a linked list of (u64 hash, char *key, void *value)
  // where the keys all have the same hash
  BucketItem *buckets[NUM_BUCKETS];
};

u64 HashMap_hash(char *str, u32 key_length) { // TODO
  u64 res = 0;
  for (int i = 0; i < key_length; i++) {
    res += str[i];
  }
  return res;
}

HashMap *HashMap_create() {
  return calloc(sizeof(HashMap), 1);
}

void HashMap_destroy(HashMap *hm) {
  for (int i = 0; i < NUM_BUCKETS; i++) {
    // TODO
  }
  free(hm);
}

BucketItem *BucketItem_create(u64 hash, char *key, u32 key_length, void *value) {
    BucketItem *item = malloc(sizeof(BucketItem));
    item->hash = hash;
    item->key = key;
    item->key_length = key_length;
    item->value = value;
    item->next = NULL;
    return item;
}

void HashMap_insert(HashMap *map, char *key, u32 key_length, void *value) {
  assert(map);

  u64 h = HashMap_hash(key, key_length);
  u64 index = h % NUM_BUCKETS;
  BucketItem *current_item = map->buckets[index];
  if (current_item == NULL) {
    map->buckets[index] = BucketItem_create(h, key, key_length, value);
    return;
  }

  while (current_item->next != NULL) {
    if (current_item->hash == h &&
        key_length == current_item->key_length &&
        strncmp(key, current_item->key, key_length) == 0) {
      free(current_item->value);
      current_item->value = value;
      return; // Key already exists
    }
    current_item = current_item->next;
  }

  if (current_item->hash == h &&
      key_length == current_item->key_length &&
      strncmp(key, current_item->key, key_length) == 0) {
    free(current_item->value);
    current_item->value = value;
    return; // Key already exists
  }

  current_item->next = BucketItem_create(h, key, key_length, value);
}

void HashMap_delete(HashMap *map, char *key, u32 key_length) {
  assert(map);

  u64 h = HashMap_hash(key, key_length);
  u64 index = h % NUM_BUCKETS;

  BucketItem *current_item = map->buckets[index];
  BucketItem *prev_item = NULL;
  while (current_item != NULL) {
    if (current_item->hash == h &&
        key_length == current_item->key_length &&
        strncmp(key, current_item->key, key_length) == 0) {
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

void *HashMap_get(HashMap *map, char *key, u32 key_length) {
  assert(map);
  u64 h = HashMap_hash(key, key_length);
  u64 index = h % NUM_BUCKETS;

  BucketItem *current_item = map->buckets[index];
  for (; current_item != NULL; current_item = current_item->next) {
    if (current_item->hash == h &&
        key_length == current_item->key_length &&
        strncmp(key, current_item->key, key_length) == 0) {
      return current_item->value;
    }
  }

  return NULL;
}

