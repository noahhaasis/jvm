#ifndef BUFFER_H
#define BUFFER_H

#include <stdlib.h>
#include <stdio.h>

#define null 0

typedef struct {
  int length;
  int capacity;
} sb_header;

#define INITIAL_CAPACITY 8

#define __buf_header(buf) ((sb_header*)((char *) buf - sizeof(sb_header)))

// TODO: Move resizing into a function since it's the rare case
#define sb_push(buffer, elem) \
  do { \
    if (!buffer) { \
      void *base = malloc(sizeof(sb_header) + INITIAL_CAPACITY * sizeof(elem)); \
      *((sb_header *)base) = (sb_header) {.length = 1, .capacity = INITIAL_CAPACITY}; \
      buffer = (((char *) base) + sizeof(sb_header)); \
      buffer[0] = elem; \
    } else { \
      sb_header *base = __buf_header(buffer); \
      if (base->length == base->capacity) { \
        base = realloc(base, sizeof(sb_header) + base->capacity * 2 * sizeof(elem)); \
        base->capacity *= 2; \
        buffer = (((char *) base) + sizeof(sb_header)); \
      }\
      buffer[base->length] = elem; \
      base->length += 1; \
    } \
  } while(0);

#define sb_length(buffer) \
  (buffer == null ? 0 : __buf_header(buffer)->length)

// TODO: Return the old top
#define sb_pop(buffer) \
  __buf_header(buffer)->length -= 1;

#define sb_free(buffer) \
  do { \
    free(__buf_header(buffer)); \
    buffer = null; \
  } while (0);

#endif
