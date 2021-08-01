#include "string.h"

#include <stdlib.h>
#include <string.h>

char *String_to_null_terminated(String string) {
  char *dest = malloc(string.length + 1);
  memcpy(dest, string.bytes, string.length + 1);
  dest[string.length] = '\0';
  return dest;
}

String String_from_null_terminated(char *str) {
   String string = (String) { .length = strlen(str) };
   string.bytes = malloc(string.length);
   memcpy(string.bytes, str, string.length);
   return string;
}
