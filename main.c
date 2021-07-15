#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "jvm.h"

code_attribute *find_code(ClassFile *class_file, method_info method_info) {
  for (int i = 0; i < method_info.attributes_count; i++) {
    attribute_info info = method_info.attributes[i];
    cp_info name_constant = class_file->constant_pool[info.attribute_name_index-1];

    if (strncmp(
          "Code",
          (const char *)name_constant.info.utf8_info.bytes,
          name_constant.info.utf8_info.length) == 0) {
      return info.info.code_attribute;
    }
  }

  printf("Code attribute not found\n");
  return NULL;
}

method_info find_method(ClassFile *class_file, char *name) {
  for (int i = 0; i < class_file->methods_count; i++) {
    method_info info = class_file->methods[i];
    cp_info name_constant = class_file->constant_pool[info.name_index - 1];

    if (strncmp(
          name,
          (const char *)name_constant.info.utf8_info.bytes,
          name_constant.info.utf8_info.length) == 0) {
      printf("Found fac\n");
      return info;
    }
  }
  printf("Failed to find method \"%s\"", name);
  assert(0);
  return (method_info){};
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Usage: %s <classfile>\n", argv[0]);
    return -1;
  }

  ClassFile *class_file = parse_class_file(argv[1]);
  printf("magic: %u %x\n", class_file->magic, class_file->magic);
  printf("minor version %u\n", class_file->minor_version);
  printf("major version %u\n", class_file->major_version);
  printf("constant pool count %u\n", class_file->constant_pool_count);
  printf("method_ref %u %u %u\n", class_file->constant_pool[0].tag, class_file->constant_pool[0].info.method_ref_info.class_index, class_file->constant_pool[0].info.method_ref_info.name_and_type_index);
  printf("class %u %u\n", class_file->constant_pool[1].tag, class_file->constant_pool[1].info.class_info.name_index);
  printf("...\n");
  printf("access_flags 0x%x\n", class_file->access_flags);
  printf("this_class %d\n", class_file->this_class);
  printf("super_class %d\n", class_file->super_class);
  printf("interfaces count %d\n", class_file->interfaces_count);
  printf("fields count %d\n", class_file->fields_count);
  printf("methods count %d\n", class_file->methods_count);
  printf("attributes count %d\n", class_file->attributes_count);

  // Print the source file name
  for (int i = 0; i < class_file->attributes_count; i++) {
    if (class_file->attributes[i].type == SourceFile_attribute) {
      int sourcefile_index = class_file->attributes[i].info.sourcefile_index - 1;
      cp_info constant = class_file->constant_pool[sourcefile_index];
      printf("source file name: \"%.*s\"\n", constant.info.utf8_info.length, constant.info.utf8_info.bytes);
    }
  }

  method_info fac_info = find_method(class_file, "fac");
  code_attribute *code_attr = find_code(class_file, fac_info);
  assert(code_attr != NULL);
  execute(*code_attr);

  free_class_file(class_file);

  return 0;
}
