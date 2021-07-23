#include "jvm.h"
#include "hashmap.h"

typedef struct {
  // Map<String, class>
  HashMap *loaded_classes;

} ClassLoader;

ClassLoader ClassLoader_create() {
  return (ClassLoader) { };
}

void ClassLoader_destroy() {
}

typedef struct Class Class;
struct Class {
  // Map<string, method>
  HashMap *method_map;
  // Map<string, field>
  HashMap *field_map;

  Class *super_class;
  // TODO: constant_pool
};

char *filename_from_classname(char *class_name, u32 length) {
  int filename_length = length + 6 + 1;
  char *file_extension = ".class";
  char *filename = malloc(filename_length);

  memcpy(filename, class_name, length);
  memcpy(filename+length, file_extension, 6);
  filename[filename_length-1] = '\0';

  return filename;
}

/* Loads a class from it's class file and convert it into its runtime representation.
 * Then execute the intitialization code. (<clinit>)
 */
Class *load_and_initialize_class(ClassLoader *loader, char *class_name, u32 length) {
  char *filename = filename_from_classname(class_name, length);
  ClassFile *class_file = parse_class_file(filename);
  free(filename);

  Class *class = NULL; // TODO


  HashMap_insert(loader->loaded_classes, class_name, length, class);
  return class;
}

Class *get_class(ClassLoader *loader, char *class_name, u32 length) {
  return HashMap_get(loader->loaded_classes, class_name, length);
}
