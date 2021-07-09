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
    u1 info[];
    /* u1 info[attribute_length]; */
} attribute_info;

typedef struct {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info attributes[];
    /* attribute_info attributes[attributes_count]; */
} method_info;

typedef struct {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info attributes[];
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
      i+=1; // Long and Double constants take up 8 bytes and 2 slots in the constant table
    }
    data_index += byte_size;
  }

  class_file->access_flags = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  class_file->this_class = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  class_file->super_class = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;
  class_file->interfaces_count = __builtin_bswap16(*((u2 *)(data + data_index))); data_index += 2;

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

  free_class_file(class_file);

  return 0;
}
