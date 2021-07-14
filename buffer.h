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

/*
int main(void) {
  int *buffer = null;
  sb_push(buffer, 1);
  sb_push(buffer, 2);
  sb_push(buffer, 3);
  sb_push(buffer, 4);
  sb_push(buffer, 5);
  sb_push(buffer, 6);
  sb_push(buffer, 7);
  sb_push(buffer, 8);
  sb_push(buffer, 9);
  printf("length: %d\n", sb_length(buffer));

  for (int i = 0; i < sb_length(buffer); i++) {
    printf("%d\n", buffer[i]);
  }

  sb_pop(buffer);

  for (int i = 0; i < sb_length(buffer); i++) {
    printf("%d\n", buffer[i]);
  }
  sb_free(buffer);


  char *anotherBuffer = null;
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'a');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'q');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'f');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'c');
  sb_push(anotherBuffer, 'a');

  sb_free(anotherBuffer);
}
*/
#endif
