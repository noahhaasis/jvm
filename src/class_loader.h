#ifndef CLASSLOADER_H
#define CLASSLOADER_H

#include "hashmap.h"
#include "class_file.h"

typedef struct {
  // Map<String, class>
  HashMap *loaded_classes;
} ClassLoader;

ClassLoader ClassLoader_create();

typedef struct {
  cp_info *constant_pool;
  code_attribute *code_attr;
  method_descriptor descriptor;
  // TODO(noah): Make this a pointer?
  u16 name_index; // 0 based
} Method;

typedef struct Class Class;
struct Class {
  // Let's keep the original ClassFile around
  ClassFile *source_class_file; // TODO: Actually see what we need

  // Map<string, Method>
  HashMap *method_map;

  // Map<string, field>
  HashMap *field_map;
};

Class *load_and_initialize_class(ClassLoader loader, char *class_name, u32 length);
Class *load_and_initialize_class_from_file(
    ClassLoader loader,
    char *class_name, u32 class_name_length,
    char *filename, u32 file_name_length);

Class *get_class(ClassLoader loader, char *class_name, u32 length);

#endif
