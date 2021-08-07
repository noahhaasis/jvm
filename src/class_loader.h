#ifndef CLASSLOADER_H
#define CLASSLOADER_H

#include "class_file.h"
#include "HashMap.h"

struct Class;

struct ClassLoader {
  // List of directories. Not owned
  char **classpath;
  u64 num_paths;

  // Map<String, class>
  HashMap<Class> *loaded_classes;
};

ClassLoader ClassLoader_create(char **classpath, u64 num_paths);

Class *load_class(ClassLoader loader, String classname);

Class *get_class(ClassLoader loader, String classname);

struct Method {
  cp_info *constant_pool;
  code_attribute *code_attr;
  method_descriptor descriptor;
  // TODO(noah): Make this a pointer?
  u16 name_index; // 0 based
};

struct FieldInfo {
  u64 index;
  // TODO: Field type
};

struct Class {
  // Let's keep the original ClassFile around
  ClassFile *source_class_file; // TODO: Actually see what we need

  // Map<string, Method>
  HashMap<Method> *method_map;

  // Map<string, field>
  HashMap<u64> *static_field_map;

  // Map<String, u64>
  HashMap<FieldInfo> *instance_field_map;
};

void set_static(Class *cls, String fieldname, u64 value);
u64 get_static(Class *cls, String fieldname);

#endif
