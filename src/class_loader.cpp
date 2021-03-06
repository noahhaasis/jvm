#include "class_loader.h"

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

ClassLoader ClassLoader_create(char **classpath, u64 num_paths) {
  return (ClassLoader) {
    .classpath = classpath,
    .num_paths = num_paths,
    .loaded_classes = new HashMap<Class>(),
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
  descriptor.parameter_types = Vector<JavaType>();

  assert(src.length >= 3);
  assert(src.bytes[0] == '(');

  int offset = 1;
  while (src.bytes[offset] != ')') {
    JavaType param_type;

    String substring = (String) {.length = src.length-offset, .bytes=src.bytes+offset};
    offset += JavaType::parse_single(substring, &param_type);
    descriptor.parameter_types.push(param_type);
  }

  assert(src.bytes[offset] == ')');

  ++offset; // Advance past ')';

  switch(src.bytes[offset]) {
  case 'V':
  {
    descriptor.return_type = (JavaType) { void_t };
    offset += 1;
  } break;
  default:
  {
    JavaType return_type;
    String substring = (String) {.length = src.length-offset, .bytes=src.bytes+offset};
    offset += JavaType::parse_single(substring, &descriptor.return_type);
  }
  }

  assert(offset == src.length);

  return descriptor;
}

code_attribute *find_code(cp_info *constant_pool, method_info method_info) {
  for (int i = 0; i < method_info.attributes_count; i++) {
    Utf8Info *attribute_name = method_info.attributes[i].attribute_name;

    if (strncmp(
          "Code",
          (const char *)attribute_name->bytes,
          attribute_name->length) == 0) {
      return method_info.attributes[i].as.code_attr;
    }
  }

  // printf("Code attribute not found\n");
  return NULL;
}

Class *Class_from_class_file(ClassLoader class_loader, ClassFile *class_file) {
  Class *cls = (Class *)malloc(sizeof(Class)); // TODO
  cls->source_class_file = class_file;
  cls->method_map = new HashMap<Method>();
  cls->static_field_map = new HashMap<u64>();
  cls->instance_field_map = new HashMap<FieldInfo>();


  // Load the superclass
  Utf8Info *super_class_name = class_file->constant_pool[class_file->super_class-1].as.class_info.name;
  cls->super_class = get_or_load_class(class_loader, (String) {
      .length = super_class_name->length,
      .bytes = (char *)super_class_name->bytes,
    });

  // Insert all methods into a hashtable
  for (int i = 0; i < class_file->methods_count; i++) {
    Method *method = (Method *)malloc(sizeof(Method));
    method->constant_pool = class_file->constant_pool;

    method_info info = class_file->methods[i];
    method->name = info.name;

    method->descriptor = parse_method_descriptor(
        (String) {
          .length = info.descriptor->length,
          .bytes = (char *)info.descriptor->bytes,
        });
    method->code_attr = find_code(class_file->constant_pool, info);

    cls->method_map->insert(
        (String) {
          .length = method->name->length,
          .bytes = (char *)method->name->bytes,
        },
        method);
  }

  // If this class has a superclass then reserve space for it's fields
  u64 field_byte_offset = cls->super_class ? cls->super_class->object_body_size : 0;
  for (u32 i = 0; i < class_file->fields_count; i++) {
    field_info field = class_file->fields[i];

    if (field.access_flags & FIELD_ACC_STATIC) {
      // TODO: Handle static field
    } else {
      JavaType field_type = JavaType::parse(
          (String) {
            .length = field.descriptor->length,
            .bytes = (char *)field.descriptor->bytes,
          });
      // TODO: Support different sized types
      u16 field_size = 8;

      FieldInfo *fieldInfo = (FieldInfo *)malloc(sizeof(FieldInfo));
      *fieldInfo = {field_byte_offset};

      cls->instance_field_map->insert((String) {
            .length = field.name->length,
            .bytes = (char *)field.name->bytes,
          }, fieldInfo);

      field_byte_offset += field_size;
    }
  }

  cls->object_body_size = field_byte_offset;

  return cls;
}

char *filename_from_classname(String classname) {
  char *file_extension = (char *)".class";
  int filename_length = classname.length + strlen(file_extension) + 1;
  char *filename = (char *)malloc(filename_length);

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
        
        char *full_path = (char *)malloc(dir_len + classfile_name_len + 2);
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

void execute(ClassLoader, Method *);

/* Loads a class from it's class file and convert it into its runtime representation.
 */
internal Class *load_class(ClassLoader loader, String classname) {
  char *path = find_classfile_in_classpath(loader, classname);
  if (!path) {
    return NULL;
  }
  ClassFile *class_file = parse_class_file(path);
  free(path);

  if (!class_file) return NULL;

  Class *cls = Class_from_class_file(loader, class_file);
  loader.loaded_classes->insert(classname, cls);

  return cls;
}

internal Class *get_class(ClassLoader loader, String classname) {
  return loader.loaded_classes->get(classname);
}

Class *get_or_load_class(ClassLoader class_loader, String classname) {
  Class * cls = get_class(class_loader, classname);
  if (cls) {
    return cls;
  }

  cls = load_class(class_loader, classname);
  if (!cls) {
    // fprintf(stderr, "Failed to find class \"%.*s\"\n", classname.length, classname.bytes);
    return NULL;
  }

  Method *clinit = cls->method_map->get(
    (String) {
      .length = strlen("<clinit>"),
      .bytes = (char *)"<clinit>",
    });
  if (clinit) {
    execute(class_loader, clinit);
  }

  return cls;
}

void set_static(Class *cls, String fieldname, u64 value) {
  // Note: Since the only values we store at the moment are 32 bits
  // we can just store them instead of the pointer.
  //
  // FIXME: insert may try to free these values. Heap allocate for now and write a non owning map later
  u64 *heap_value = (u64 *)malloc(sizeof(u64));
  *heap_value = value;
  cls->static_field_map->insert(fieldname, heap_value);
}

u64 get_static(Class *cls, String fieldname) {
  return *cls->static_field_map->get(fieldname);
}
