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
  // TODO
}

typedef struct {
  code_attribute *code_attr;
  method_descriptor *descriptor;
  u16 name_index;
} Method;

typedef struct Class Class;
struct Class {
  // Let's keep the original ClassFile around
  ClassFile *source_class_file;

  // Map<string, Method>
  HashMap *method_map;

  // Map<string, field>
  HashMap *field_map;
};

Class *Class_from_class_file(ClassFile *class_file) {
  Class *class = malloc(sizeof(Class)); // TODO
  class->source_class_file = class_file;

  return class;
}

// TODO:
// Class_get_method(char *method_name, u32 method_name_length)

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

  Class *class = Class_from_class_file(class_file);

  HashMap_insert(loader->loaded_classes, class_name, length, class);

  // TODO: Call <clinit>

  return class;
}

Class *get_class(ClassLoader *loader, char *class_name, u32 length) {
  return HashMap_get(loader->loaded_classes, class_name, length);
}
