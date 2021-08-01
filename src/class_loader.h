#ifndef CLASSLOADER_H
#define CLASSLOADER_H

#include "hashmap.h"
#include "class_file.h"

typedef struct Class Class;

typedef struct {
  // Map<String, class>
  HashMap *loaded_classes;
} ClassLoader;

ClassLoader ClassLoader_create();

Class *load_class(ClassLoader loader, String classname);
Class *load_class_from_file(ClassLoader loader, String classname, String filename);

Class *get_class(ClassLoader loader, String classname);

typedef struct {
  cp_info *constant_pool;
  code_attribute *code_attr;
  method_descriptor descriptor;
  // TODO(noah): Make this a pointer?
  u16 name_index; // 0 based
} Method;

struct Class {
  // Let's keep the original ClassFile around
  ClassFile *source_class_file; // TODO: Actually see what we need

  // Map<string, Method>
  HashMap *method_map;

  // Map<string, field>
  HashMap *field_map;
};

void set_static(Class *class, String fieldname, u32 value);
u32 get_static(Class *class, String fieldname);

#endif
