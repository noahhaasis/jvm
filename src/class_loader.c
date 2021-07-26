#include "class_loader.h"

#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

ClassLoader ClassLoader_create() {
  return (ClassLoader) {
    .loaded_classes = HashMap_create()
  };
}

void ClassLoader_destroy() {
  // TODO
}

method_descriptor parse_method_descriptor(char *src, int length) {
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

  assert(length >= 3);
  assert(src[0] == '(');

  int offset = 1;
  for (; offset < (length - 1) && src[offset] != ')'; ++offset) {
    switch(src[offset]) {
    case 'I':
    {
      descriptor.all_params_byte_count += 4;
      sb_push(params, int_t);
    } break;
    case 'D':
    {
      descriptor.all_params_byte_count += 8;
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

  assert(src[offset] == ')');

  ++offset; // Advance past ')';

  switch(src[offset]) {
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

  assert((offset + 1) == length);

  return descriptor;
}

char *allocate_null_terminated_string(char *src, u32 length) {
  char *dest = malloc(length + 1);
  memcpy(dest, src, length);
  dest[length] = '\0';
  return dest;
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
        (char *)descriptor_string_constant.as.utf8_info.bytes,
        descriptor_string_constant.as.utf8_info.length);
    method->code_attr = find_code(class_file->constant_pool, info);

    cp_info method_name_constant = class_file->constant_pool[method->name_index];
    HashMap_insert(
        class->method_map,
        (char *)method_name_constant.as.utf8_info.bytes,
        method_name_constant.as.utf8_info.length,
        method);
  }

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
Class *load_class(ClassLoader loader, char *class_name, u32 length) {
  char *filename = filename_from_classname(class_name, length);
  ClassFile *class_file = parse_class_file(filename);
  free(filename);

  if (!class_file) return NULL;

  Class *class = Class_from_class_file(class_file);

  HashMap_insert(loader.loaded_classes, class_name, length, class);

  return class;
}

Class *load_class_from_file(
    ClassLoader loader,
    char *class_name, u32 class_name_length,
    char *filename, u32 filename_length) {
  char *filename_nt = allocate_null_terminated_string(filename, filename_length);
  ClassFile *class_file = parse_class_file(filename_nt);
  free(filename_nt);
  if (!class_file) return NULL;

  Class *class = Class_from_class_file(class_file);

  HashMap_insert(loader.loaded_classes, class_name, class_name_length, class);

  return class;
}

Class *get_class(ClassLoader loader, char *class_name, u32 length) {
  return HashMap_get(loader.loaded_classes, class_name, length);
}
