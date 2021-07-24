#ifndef JVM_H
#define JVM_H

#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>


/* NOTE(Noah):
 * Multiple byte values in java class files are always stored in _big-endian order_
 *
 * Also class file indices into the constant pool are 1 based.
 * So be careful to always use constant_pool[some_index-1]
 */

#define CONSTANT_Class                7u
#define CONSTANT_Fieldref             9u
#define CONSTANT_Methodref            10u
#define CONSTANT_InterfaceMethodref   11u
#define CONSTANT_String               8u
#define CONSTANT_Integer              3u
#define CONSTANT_Float                4u
#define CONSTANT_Long                 5u
#define CONSTANT_Double               6u
#define CONSTANT_NameAndType          12u
#define CONSTANT_Utf8                 1u
#define CONSTANT_MethodHandle         15u
#define CONSTANT_MethodType           16u
#define CONSTANT_InvokeDynamic        18u

void pretty_print_constant_tag(u8 tag);

typedef struct {
    u8 tag;
    union {
      struct {
        u16 name_index;
      } class_info;

      struct {
        u16 class_index;
        u16 name_and_type_index;
      } field_ref_info;

      struct {
        u16 class_index;
        u16 name_and_type_index;
      } methodref_info;

      struct {
        u16 class_index;
        u16 name_and_type_index;
      } interface_methodref_info;

      struct {
        u16 string_index;
      } string_info;

      struct {
        u32 bytes;
      } integer_info;

      float  float_value;
      i64    long_value;
      double double_value;

      struct {
        u16 name_index;
        u16 descriptor_index;
      } name_and_type_info;

      struct {
        u16 length;
        u8* bytes;
      } utf8_info;

      struct {
        u8 reference_kind;
        u16 reference_index;
      } method_handle_info;

      struct {
        u16 descriptor_index;
      } method_type_info;

      struct {
        u16 bootstrap_method_attr_index;
        u16 name_and_type_index;
      } invoke_dynamic_info;
    } as;
    // TODO(noah): rename "info" to "as"
    /*u8 info[]*/;
} cp_info;

typedef struct {
  u16 start_pc;
  u16 end_pc;
  u16 handler_pc;
  u16 catch_type;
} exception_table_entry;

typedef enum {
  iconst        = 2,  /* 2 - 8 ; Push iconst_<n>*/
  ldc           = 18,
  iload         = 21,
  aload         = 25,
  iload_n       = 26, /* 26 - 29 */
  aload_n       = 42, /* 42 - 45 */
  istore        = 54,
  istore_n      = 59, /* 59 - 62 */
  pop           = 87,
  iadd          = 96,
  isub          = 100,
  imul          = 104,
  iinc          = 132,
  ifeq          = 153,
  ifne          = 154,
  if_icmpne     = 160,
  if_icmpgt     = 163,
  goto_instr    = 167,
  ireturn       = 172,
  return_void   = 177,
  getstatic     = 178,
  putstatic     = 179,
  invokevirtual = 182,
  invokespecial = 183,
  invokestatic  = 184,
  undefined,
} instruction_type;

typedef struct attribute_info attribute_info;

typedef struct {
  u16 max_stack;
  u16 max_locals;
  u32 code_length;
  u8 *code;
  /* u8 code[code_length]; */
  u16 exception_table_length;
  exception_table_entry *exception_table;
  /* exception_table[exception_table_length]; */
  u16 attributes_count;
  attribute_info *attributes;
  /* attribute_info attributes[attributes_count]; */
} code_attribute;

typedef enum {
  SourceFile_attribute,
  ConstantValue_attribute,
  Code_attribute,
  LineNumberTable_attribute,
  StackMapTable_attribute,
  Unknown_attribute
} attribute_type;

struct attribute_info {
    u16 attribute_name_index;
    u32 attribute_length;
    attribute_type type;
    union {
      /* SourceFile_attribute */
      u16 sourcefile_index;

      /* ConstantValue_attribute */
      u16 constantvalue_index;

      /* Code_attribute */
      code_attribute *code_attribute;

      /* other */
      u8* bytes;
    } as;
    // u8 *info;
    /* u8 info[attribute_length]; */
};

typedef struct {
    u16             access_flags;
    u16             name_index;
    u16             descriptor_index;
    u16             attributes_count;
    attribute_info *attributes;
    /* attribute_info attributes[attributes_count]; */
} method_info;

typedef enum {
  int_t = 1,
  double_t,
} parameter_descriptor;

typedef enum {
  void_t = 0,
  // ... parameter_descriptor
} return_descriptor;

typedef struct {
  u32 all_params_byte_count;
  parameter_descriptor *parameter_types; /* stretchy buffer */
  return_descriptor return_type;
} method_descriptor;

typedef struct {
    u16             access_flags;
    u16             name_index;
    u16             descriptor_index;
    u16             attributes_count;
    attribute_info *attributes;
    /* attribute_info attributes[attributes_count]; */
} field_info;

/* class access_flags */
#define ACC_PUBLIC      0x0001u
#define ACC_FINAL       0x0010u
#define ACC_SUPER       0x0020u
#define ACC_INTERFACE   0x0200u
#define ACC_ABSTRACT    0x0400u
#define ACC_SYNTHETIC   0x1000u
#define ACC_ANNOTATION  0x2000u
#define ACC_ENUM        0x4000u

typedef struct {
  u32             magic;
  u16             minor_version;
  u16             major_version;
  u16             constant_pool_count;
  cp_info        *constant_pool;
  /* cp_info        constant_pool[constant_pool_count-1]; */
  u16             access_flags;
  u16             this_class;
  u16             super_class;
  u16             interfaces_count;
  u16             *interfaces;
  /* u16             interfaces[interfaces_count]; */
  u16             fields_count;
  field_info     *fields;
  /* field_info     fields[fields_count]; */
  u16             methods_count;
  method_info    *methods;
  /* method_info    methods[methods_count]; */
  u16             attributes_count;
  attribute_info *attributes;
  /* attribute_info attributes[attributes_count]; */
} ClassFile;

attribute_type parse_attribute_type(char *unicode_name, int length);

attribute_info parse_attribute_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */);

method_info parse_method_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */);

field_info parse_field_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */);

cp_info parse_cp_info(u8 *data, int *out_byte_size /* How many bytes were parsed */);

ClassFile *parse_class_file(char *filename);

void free_class_file(ClassFile *class_file);

code_attribute *find_code(ClassFile *class_file, method_info method_info);

method_info find_method(ClassFile *class_file, char *name, u32 name_length);

void execute(ClassFile *class_file, code_attribute method_code);

#endif
