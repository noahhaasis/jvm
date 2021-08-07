#ifndef STRING_H
#define STRING_H

#include "common.h"

struct String {
  u64 length;
  char *bytes;
};

char *String_to_null_terminated(String str);

String String_from_null_terminated(char *str);

#endif
