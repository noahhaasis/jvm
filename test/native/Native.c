#include "Native.h"
#include <stdlib.h>

JNIEXPORT void JNICALL Java_Native_sayHello
  (JNIEnv *, jobject) {
  printf("Hi there!\n");
}  

