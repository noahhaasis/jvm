#ifndef CLASSLOADER_H
#define CLASSLOADER_H

#include "class_file.h"
#include "HashMap.h"

struct Class;

struct ClassLoader {
  // List of directories. Not owned
  char **classpath;
  u64 num_paths;

  HashMap<Class> *loaded_classes;
};

ClassLoader ClassLoader_create(char **classpath, u64 num_paths);

/* Return a already loaded class or load the class from it's
 * class file and execute <clinit>
 */
Class *get_or_load_class(ClassLoader class_loader, String classname);

struct Method {
  cp_info *constant_pool;
  code_attribute *code_attr;
  method_descriptor descriptor;
  Utf8Info *name;
};

struct FieldInfo {
  u64 index;
  // TODO: Field type
};

struct Class {
  // Let's keep the original ClassFile around
  ClassFile *source_class_file; // TODO: Actually see what we need

  Class *super_class;

  HashMap<Method> *method_map;

  HashMap<u64> *static_field_map;

  HashMap<FieldInfo> *instance_field_map;

  u32 object_body_size;
};

void set_static(Class *cls, String fieldname, u64 value);
u64 get_static(Class *cls, String fieldname);

#endif
