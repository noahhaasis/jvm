#ifndef STRING_H
#define STRING_H

#include "common.h"

typedef struct {
  u32 size;
  u8 *bytes;
} String;

char *String_to_null_terminated(String str);

String String_from_null_terminated(char *str);

#endif
