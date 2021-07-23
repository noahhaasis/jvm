#include "jvm.h"

typedef struct {

} class_loader;

class_loader create_class_loader() {
  return (class_loader) { };
}

void destroy_class_loader() {
}

typedef struct {
  // methods
  // fields
  // pointer to parent
} class;

void load_class(class_loader *loader, char *class_name, int length) {
}

class *get_class(class_loader *loader, char *class_name, int length) {
  return NULL;
}
