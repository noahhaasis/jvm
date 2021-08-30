#pragma once

#include "string.h"
#include "stdlib.h"
#include <string.h>

constexpr u64 NUM_BUCKETS = 64;


template<typename T>
struct BucketItem {
  u64 hash;
  String key;
  T *value;
  BucketItem *next;
};

template<typename T>
internal BucketItem<T> *BucketItem_create(u64 hash, String key, T *value) {
    BucketItem<T> *item = (BucketItem<T> *)malloc(sizeof(BucketItem<T>));
    item->hash = hash;
    item->key = key;
    item->value = value;
    item->next = NULL;
    return item;
}

// TODO: Create specialization: HashMap<u64>

template<typename T>
class HashMap {
public:
  void insert(String key, T *value) {
    u64 h = hash(key);
    u64 index = h % NUM_BUCKETS;
    BucketItem<T> *current_item = this->m_buckets[index];
    if (current_item == NULL) {
      this->m_buckets[index] = BucketItem_create(h, key, value);
      return;
    }

    while (current_item->next != NULL) {
      if (current_item->hash == h &&
          key.length == current_item->key.length &&
          strncmp(key.bytes, current_item->key.bytes, key.length) == 0) {
        free(current_item->value);
        current_item->value = value;
        return; // Key already exists
      }
      current_item = current_item->next;
    }

    if (current_item->hash == h &&
        key.length == current_item->key.length &&
        strncmp(key.bytes, current_item->key.bytes, key.length) == 0) {
      free(current_item->value);
      current_item->value = value;
      return; // Key already exists
    }

    current_item->next = BucketItem_create(h, key, value);
  }

  // Heap allocates value
  void insert(String key, T value) {
    T *heap_value = malloc(sizeof(T));
    *heap_value = value;
    this->insert(key, heap_value);
  }

  T *get(String key) {
    u64 h = hash(key);
    u64 index = h % NUM_BUCKETS;

    BucketItem<T> *current_item = this->m_buckets[index];
    for (; current_item != NULL; current_item = current_item->next) {
      if (current_item->hash == h &&
          key.length == current_item->key.length &&
          strncmp(key.bytes, current_item->key.bytes, key.length) == 0) {
        return current_item->value;
      }
    }

    return NULL;
  }


private:
  // Each bucket is a linked list of (u64 hash, char *key, void *value)
  // where the keys all have the same hash
  BucketItem<T> *m_buckets[NUM_BUCKETS];

  static u64 hash(String key) { // TODO
    u64 res = 0;
    for (int i = 0; i < key.length; i++) {
      res += key.bytes[i];
    }
    return res;
  }

  // TODO(noah):
  // Use a stretchy buffer instead of a linked list?
  // And then maybe split the hashes into a seperate buffer (data-oriented design)
};
