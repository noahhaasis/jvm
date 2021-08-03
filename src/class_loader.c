#include "class_loader.h"

#include "buffer.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

ClassLoader ClassLoader_create(char **classpath, u64 num_paths) {
  return (ClassLoader) {
    .classpath = classpath,
    .loaded_classes = HashMap_create(),
    .num_paths = num_paths
  };
}

void ClassLoader_destroy() {
  // TODO
}

method_descriptor parse_method_descriptor(String src) {
    /* Grammar:
    MethodDescriptor:
        ( ParameterDescriptor* ) ReturnDescriptor

    ParameterDescriptor:
        FieldType

    ReturnDescriptor:
        FieldType
        VoidDescriptor

    VoidDescriptor:
        V
    */
  method_descriptor descriptor = (method_descriptor){ };
  parameter_descriptor *params = NULL;

  assert(src.length >= 3);
  assert(src.bytes[0] == '(');

  int offset = 1;
  for (; offset < (src.length - 1) && src.bytes[offset] != ')'; ++offset) {
    switch(src.bytes[offset]) {
    case 'I':
    {
      sb_push(params, int_t);
    } break;
    case 'D':
    {
      sb_push(params, double_t);
    } break;
    case '[':
    {
      // TODO
      return descriptor;
    } break;
    default:
    {
      printf("Failed to parse parameter type. Rest of signature TODO");
      assert(0);
    }
    }
  }

  descriptor.parameter_types = params;

  assert(src.bytes[offset] == ')');

  ++offset; // Advance past ')';

  switch(src.bytes[offset]) {
  case 'V':
  {
    descriptor.return_type = void_t;
  } break;
  case 'I':
  {
    descriptor.return_type = int_t;
  } break;
  case 'D':
  {
    descriptor.return_type = double_t;
  } break;
  default:
  {
      printf("Failed to parse return type. Rest of signature TODO");
      assert(0);
  }
  }

  assert((offset + 1) == src.length);

  return descriptor;
}

code_attribute *find_code(cp_info *constant_pool, method_info method_info) {
  for (int i = 0; i < method_info.attributes_count; i++) {
    attribute_info info = method_info.attributes[i];
    cp_info name_constant = constant_pool[info.attribute_name_index-1];

    if (strncmp(
          "Code",
          (const char *)name_constant.as.utf8_info.bytes,
          name_constant.as.utf8_info.length) == 0) {
      return info.as.code_attribute;
    }
  }

  printf("Code attribute not found\n");
  return NULL;
}

Class *Class_from_class_file(ClassFile *class_file) {
  Class *class = malloc(sizeof(Class)); // TODO
  class->source_class_file = class_file;
  class->method_map = HashMap_create();
  class->field_map = HashMap_create();


  for (int i = 0; i < class_file->methods_count; i++) {
    Method *method = malloc(sizeof(Method));
    method->constant_pool = class_file->constant_pool;

    method_info info = class_file->methods[i];
    method->name_index = info.name_index - 1;

    cp_info descriptor_string_constant = class_file->constant_pool[info.descriptor_index-1];
    method->descriptor = parse_method_descriptor(
        (String) {
          .bytes = (char *)descriptor_string_constant.as.utf8_info.bytes,
          .length = descriptor_string_constant.as.utf8_info.length
        });
    method->code_attr = find_code(class_file->constant_pool, info);

    cp_info method_name_constant = class_file->constant_pool[method->name_index];
    HashMap_insert(
        class->method_map,
        (String) {
          .bytes = (char *)method_name_constant.as.utf8_info.bytes,
          .length = method_name_constant.as.utf8_info.length
        },
        method);
  }

  return class;
}

char *filename_from_classname(String classname) {
  char *file_extension = ".class";
  int filename_length = classname.length + strlen(file_extension) + 1;
  char *filename = malloc(filename_length);

  memcpy(filename, classname.bytes, classname.length);
  memcpy(filename + classname.length, file_extension, strlen(file_extension));
  filename[filename_length-1] = '\0';

  return filename;
}

char *find_classfile_in_classpath(ClassLoader loader, String classname) {
  char *classfile_name = filename_from_classname(classname);

  for (u32 i = 0; i < loader.num_paths; ++i) {
    char *dir_path = loader.classpath[i];

    DIR *dirp = opendir(dir_path);
    struct dirent *file_info = NULL;
    while ((file_info = readdir(dirp))) {
      if (strcmp(classfile_name, file_info->d_name) == 0) {
        u32 dir_len = strlen(dir_path);
        u32 classfile_name_len = strlen(classfile_name);
        
        char *full_path = malloc(dir_len + classfile_name_len + 2);
        memcpy(full_path, dir_path, dir_len);
        full_path[dir_len] = '/';
        memcpy(full_path + dir_len + 1, classfile_name, classfile_name_len);
        full_path[dir_len+classfile_name_len+1] = '\0';

        return full_path;
      }
    }
  }

  free(classfile_name);
  return NULL;
}

/* Loads a class from it's class file and convert it into its runtime representation.
 */
Class *load_class(ClassLoader loader, String classname) {
  char *path = find_classfile_in_classpath(loader, classname);
  if (!path) {
    return NULL;
  }
  ClassFile *class_file = parse_class_file(path);
  free(path);

  if (!class_file) return NULL;

  Class *class = Class_from_class_file(class_file);
  HashMap_insert(loader.loaded_classes, classname, class);

  return class;
}

Class *get_class(ClassLoader loader, String classname) {
  return HashMap_get(loader.loaded_classes, classname);
}

void set_static(Class *class, String fieldname, u32 value) {
  // Note: Since the only values we store at the moment are 32 bits
  // we can just store them instead of the pointer.
  //
  // FIXME: insert may try to free these values. Heap allocate for now and write a non owning map later
  HashMap_insert(class->field_map, fieldname, (void *)value);
}

u32 get_static(Class *class, String fieldname) {
  return (u32) HashMap_get(class->field_map, fieldname);
}
