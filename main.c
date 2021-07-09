#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;

/* NOTE(Noah):
 * Multiple byte values in java class files are always stored in _big-endian order_
 */

#define CONSTANT_Class 	              7u
#define CONSTANT_Fieldref 	          9u
#define CONSTANT_Methodref 	          10u
#define CONSTANT_InterfaceMethodref 	11u
#define CONSTANT_String 	            8u
#define CONSTANT_Integer 	            3u
#define CONSTANT_Float 	              4u
#define CONSTANT_Long 	              5u
#define CONSTANT_Double 	            6u
#define CONSTANT_NameAndType 	        12u
#define CONSTANT_Utf8 	              1u
#define CONSTANT_MethodHandle 	      15u
#define CONSTANT_MethodType 	        16u
#define CONSTANT_InvokeDynamic 	      18u

void pretty_print_constant_tag(u1 tag) {
  switch(tag) {
    case CONSTANT_Class: printf("Class\n"); break;
    case CONSTANT_Fieldref: printf("Fieldref\n"); break;
    case CONSTANT_Methodref: printf("Methodref\n"); break;
    case CONSTANT_InterfaceMethodref: printf("InterfaceMethodref\n"); break;
    case CONSTANT_String: printf("String\n"); break;
    case CONSTANT_Integer: printf("Integer\n"); break;
    case CONSTANT_Float: printf("Float\n"); break;
    case CONSTANT_Long: printf("Long\n"); break;
    case CONSTANT_Double: printf("Double\n"); break;
    case CONSTANT_NameAndType: printf("NameAndType\n"); break;
    case CONSTANT_Utf8: printf("Utf8\n"); break;
    case CONSTANT_MethodHandle: printf("MethodHandle\n"); break;
    case CONSTANT_MethodType: printf("MethodType\n"); break;
    case CONSTANT_InvokeDynamic: printf("InvokeDynamic\n"); break;
    default: printf("Unknown constant tag %d\n", tag);
  }

}

typedef struct {
    u1 tag;
    union {
      struct {
        u2 name_index;
      } class_info;

      struct {
        u2 class_index;
        u2 name_and_type_index;
      } field_ref_info;

      struct {
        u2 class_index;
        u2 name_and_type_index;
      } method_ref_info;

      struct {
        u2 class_index;
        u2 name_and_type_index;
      } interface_method_ref_info;

      struct {
        u2 string_index;
      } string_info;

      struct {
        u4 bytes;
      } integer_info;

      struct {
        u4 bytes; // TODO: Actually use a float?
      } float_info;

      struct { // TODO: Use a long?
        u4 high_bytes;
        u4 low_bytes;
      } long_info;

      struct { // TODO: Use a double?
        u4 high_bytes;
        u4 low_bytes;
      } double_info;

      struct {
        u2 name_index;
        u2 descriptor_index;
      } name_and_type_info;

      struct {
        u2 length;
        u1* bytes;
      } utf8_info;

      struct {
        u1 reference_kind;
        u2 reference_index;
      } method_handle_info;

      struct {
        u2 descriptor_index;
      } method_type_info;

      struct {
        u2 bootstrap_method_attr_index;
        u2 name_and_type_index;
      } invoke_dynamic_info;
    } info;
    /*u1 info[]*/;
} cp_info;

typedef struct {
    u2 attribute_name_index;
    u4 attribute_length;
    u1 *info;
    /* u1 info[attribute_length]; */
} attribute_info;

typedef struct {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info *attributes;
    /* attribute_info attributes[attributes_count]; */
} method_info;

typedef struct {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info *attributes;
    /* attribute_info attributes[attributes_count]; */
} field_info;

/* class access_flags */
#define ACC_PUBLIC 	    0x0001u
#define ACC_FINAL 	    0x0010u
#define ACC_SUPER 	    0x0020u
#define ACC_INTERFACE 	0x0200u
#define ACC_ABSTRACT 	  0x0400u
#define ACC_SYNTHETIC 	0x1000u
#define ACC_ANNOTATION 	0x2000u
#define ACC_ENUM 	      0x4000u

typedef struct {
  u4             magic;
  u2             minor_version;
  u2             major_version;
  u2             constant_pool_count;
  cp_info        *constant_pool;
  /* cp_info        constant_pool[constant_pool_count-1]; */
  u2             access_flags;
  u2             this_class;
  u2             super_class;
  u2             interfaces_count;
  u2             *interfaces;
  /* u2             interfaces[interfaces_count]; */
  u2             fields_count;
  field_info     *fields;
  /* field_info     fields[fields_count]; */
  u2             methods_count;
  method_info    *methods;
  /* method_info    methods[methods_count]; */
  u2             attributes_count;
  attribute_info *attributes;
  /* attribute_info attributes[attributes_count]; */
} ClassFile;

attribute_info parse_attribute_info(u1 *data, int *out_byte_size /* How many bytes were parsed */) {
  attribute_info info = (attribute_info) { };
  info.attribute_name_index = *((u2 *) data);
  info.attribute_length = *((u4 *) (data + 2));

  int offset = 6;

  if (info.attribute_length == 0) {
    *out_byte_size = offset;
    info.info = NULL;
    return info;
  }

  info.info = malloc(info.attribute_length);
  memcpy(info.info, data+offset, info.attribute_length);

  offset += info.attribute_length;

  *out_byte_size = offset;

  return info;
}

method_info parse_method_info(u1 *data, int *out_byte_size /* How many bytes were parsed */) {
  int offset = 0;
  method_info info = (method_info) { };

  info.access_flags = *((u2 *)data);
  info.name_index = *(((u2 *)data)+1);
  info.descriptor_index = *(((u2 *)data)+2);
  info.attributes_count = *(((u2 *)data)+3);
  offset = 8;

  // TODO(Noah): Bug we're messing something up here. This causes attribute_length to be wrong
  // in parse_attribute_info => SEGFAULT
  printf("name_index: %d; flags: 0x%x\n", info.name_index, info.access_flags);

  if (info.attributes_count == 0) {
    *out_byte_size = offset;
    info.attributes = NULL;
    return info;
  }

  info.attributes = malloc(sizeof(attribute_info) * info.attributes_count);
  for (int i = 0; i < info.attributes_count; i++) {
    int attribute_size = 0;
    info.attributes[i] = parse_attribute_info(data+offset, &attribute_size);
    offset += attribute_size;
  }

  *out_byte_size = offset;
  return info;
}

field_info parse_field_info(u1 *data, int *out_byte_size /* How many bytes were parsed */) {
  field_info info = (field_info) { };
  int offset = 0;

  info.access_flags = *((u2 *) data);
  info.name_index = *(((u2 *) data)+1);
  info.descriptor_index = *(((u2 *) data)+2);
  info.attributes_count = *(((u2 *) data)+3);
  offset += 8;

  if (info.attributes_count == 0) {
    info.attributes = NULL;
    *out_byte_size = offset;
    return info;
  }

  info.attributes = malloc(sizeof(attribute_info) * info.attributes_count);

  // TODO(Noah): BUG
  // Type mismatch. We allocate for a list of pointers but parse_attribute_info
  // returns a value. It has to be heap allocated an therefore a list of pointers.
  // Otherwise we point to some invalid part of the stack
  //
  // No. We should know how big the structures are that we return, so we can allocate a list of this structure.
  // But we don't know how much of the input we consumed
  //
  // 21:07 09 July 2021

  for (int i = 0; i < info.attributes_count; i++) {
    int attribute_size = 0;
    info.attributes[i] = parse_attribute_info(data + offset, &attribute_size);

    offset += attribute_size;
  }

  *out_byte_size = offset;

  return info;
}

cp_info parse_cp_info(u1 *data, int *out_byte_size /* How many bytes were parsed */) {
  cp_info info = (cp_info) { };
  info.tag = data[0];

  switch (info.tag) {
  case CONSTANT_Methodref:
  {
    info.info.method_ref_info.class_index = __builtin_bswap16(*((u2 *)(data+1)));
    info.info.method_ref_info.name_and_type_index = __builtin_bswap16(*((u2 *)(data+3)));
    *out_byte_size = 5;
  } break;
  case CONSTANT_Class:
  {
    info.info.class_info.name_index = __builtin_bswap16(*((u2 *)(data+1)));
    *out_byte_size = 3;
  } break;
  case CONSTANT_NameAndType:
  {
    info.info.name_and_type_info.name_index = __builtin_bswap16(*((u2 *)(data+1)));
    info.info.name_and_type_info.descriptor_index = __builtin_bswap16(*((u2 *)(data+3)));
    *out_byte_size = 5;
  } break;
  case CONSTANT_Utf8:
  {
    // TODO: Length 0? Don't malloc in this case
    info.info.utf8_info.length = __builtin_bswap16(*((u2 *)(data+1)));
    info.info.utf8_info.bytes = malloc(info.info.utf8_info.length);
    memcpy(info.info.utf8_info.bytes, data+3, info.info.utf8_info.length);
    *out_byte_size = 3 + info.info.utf8_info.length;
  } break;
  case CONSTANT_Fieldref:
  {
    info.info.field_ref_info.class_index = __builtin_bswap16(*((u2 *)(data+1)));
    info.info.field_ref_info.name_and_type_index = __builtin_bswap16(*((u2 *)(data+3)));
    *out_byte_size = 5;
  } break;
  case CONSTANT_String:
  {
    info.info.string_info.string_index = __builtin_bswap16(*((u2 *)(data+1)));
    *out_byte_size = 3;
  } break;
  case CONSTANT_Double:
  {
    info.info.double_info.high_bytes = __builtin_bswap32(*((u4 *)(data+1)));
    info.info.double_info.low_bytes = __builtin_bswap32(*((u4 *)(data+5)));
    *out_byte_size = 9;
  } break;
  default: printf("Unhandled cp_info type %u\n", info.tag); break;
  }
  return info;
}

ClassFile *parse_class_file(char *filename) {
  // mmap the whole file for now
  int fd = open(filename, O_RDONLY);
  if (!fd) return NULL;

  struct stat stat;
  fstat(fd, &stat);

  u1* data = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);

  ClassFile *class_file = malloc(sizeof(ClassFile));

  class_file->magic               = __builtin_bswap32(((u4 *)data)[0]);
  class_file->minor_version       = __builtin_bswap16(((u2 *)data)[2]);
  class_file->major_version       = __builtin_bswap16(((u2 *)data)[3]);
  class_file->constant_pool_count = __builtin_bswap16(((u2 *)data)[4]);

  class_file->constant_pool = malloc(sizeof(cp_info)*(class_file->constant_pool_count-1));
  int byte_size = 0;
  int data_index = 10; // Skip what we already parsed
  for (int i = 0; i < class_file->constant_pool_count-1; i++) {
    class_file->constant_pool[i] = parse_cp_info(data + data_index, &byte_size);
    if (class_file->constant_pool[i].tag == CONSTANT_Long
        || class_file->constant_pool[i].tag == CONSTANT_Double) {
      // TODO(Noah): Now the indexing is wrong. Will lead to problems later
      i+=1; // Long and Double constants take up 8 bytes and 2 slots in the constant table
    }
    data_index += byte_size;
  }

  class_file->access_flags = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  class_file->this_class = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  class_file->super_class = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  class_file->interfaces_count = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  if (class_file->interfaces_count > 0) {
    class_file->interfaces = malloc(class_file->interfaces_count * sizeof(u2));
    memcpy(class_file->interfaces, data + data_index, class_file->interfaces_count * sizeof(u2));
    data_index += class_file->interfaces_count * sizeof(u2);
  } else {
    class_file->interfaces = NULL;
  }
  class_file->fields_count = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  if (class_file->fields_count > 0) {
    class_file->fields = malloc(class_file->fields_count * sizeof(field_info));
    for (int i = 0; i < class_file->fields_count; i++) {
      int field_info_size = 0;
      class_file->fields[i] = parse_field_info(data+data_index, &field_info_size);
      data_index += field_info_size;
    }
  } else {
    class_file->fields = NULL;
  }

  // TODO: Somewhere here is a bug and were parsing at the wrong offset

  class_file->methods_count = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  if (class_file->methods_count == 0) {
    class_file->methods = NULL;
  } else {
    class_file->methods = malloc(sizeof(method_info) * class_file->methods_count);
    printf("methods count %d\n", class_file->methods_count);
    for (int i = 0; i < class_file->methods_count; i++) {
      int method_info_size = 0;
      class_file->methods[i] = parse_method_info(data+data_index, &method_info_size);
      data_index += method_info_size;
    }
  }

  class_file->attributes_count = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  if (class_file->attributes_count == 0) {
    class_file->attributes = NULL;
  } else {
    class_file->attributes = malloc(sizeof(attribute_info) * class_file->attributes_count);
    for (int i = 0; i < class_file->attributes_count; i++) {
      int attribute_info_size = 0;
      class_file->attributes[i] = parse_attribute_info(data+data_index, &attribute_info_size);
      data_index += attribute_info_size;
    }
  }


  close(fd);
  
  return class_file;
}

void free_class_file(ClassFile *class_file) {
  // TODO
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

  free_class_file(class_file);

  return 0;
}
