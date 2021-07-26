#include "string.h"

char *String_to_null_terminated(String string) {
  char *dest = malloc(string.size + 1);
  memcpy(dest, string.bytes, string.size + 1);
  dest[string.size] = '\0';
  free(string.bytes);
  return dest;
}

String String_from_null_terminated(char *str) {
   String string = { .size = strlen(str); }
   string.bytes = malloc(string.size);
   memcpy(string.bytes, str, string.size);
   return string;
}
