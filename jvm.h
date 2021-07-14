#ifndef JVM_H
#define JVM_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

typedef int64_t i64;


typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

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

void pretty_print_constant_tag(u1 tag);

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

      float  float_value;
      i64    long_value;
      double double_value;

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
  u2 start_pc;
  u2 end_pc;
  u2 handler_pc;
  u2 catch_type;
} exception_table_entry;

typedef struct attribute_info attribute_info;

typedef struct {
  u2 max_stack;
  u2 max_locals;
  u4 code_length;
  u1 *code;
  /* u1 code[code_length]; */
  u2 exception_table_length;
  exception_table_entry *exception_table;
  /* exception_table[exception_table_length]; */
  u2 attributes_count;
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
    u2 attribute_name_index;
    u4 attribute_length;
    attribute_type type;
    // TODO: Include the attribute kind as an enum?
    union {
      /* SourceFile_attribute */
      u2 sourcefile_index;

      /* ConstantValue_attribute */
      u2 constantvalue_index;

      /* Code_attribute */
      code_attribute *code_attribute;

      /* other */
      u1* bytes;
    } info;
    // u1 *info;
    /* u1 info[attribute_length]; */
};

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
#define ACC_PUBLIC      0x0001u
#define ACC_FINAL       0x0010u
#define ACC_SUPER       0x0020u
#define ACC_INTERFACE   0x0200u
#define ACC_ABSTRACT    0x0400u
#define ACC_SYNTHETIC   0x1000u
#define ACC_ANNOTATION  0x2000u
#define ACC_ENUM        0x4000u

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

attribute_type parse_attribute_type(char *unicode_name, int length);

attribute_info parse_attribute_info(ClassFile *class_file, u1 *data, int *out_byte_size /* How many bytes were parsed */);

method_info parse_method_info(ClassFile *class_file, u1 *data, int *out_byte_size /* How many bytes were parsed */);

field_info parse_field_info(ClassFile *class_file, u1 *data, int *out_byte_size /* How many bytes were parsed */);

cp_info parse_cp_info(u1 *data, int *out_byte_size /* How many bytes were parsed */);

ClassFile *parse_class_file(char *filename);

void free_class_file(ClassFile *class_file);

/* Next: executing simple bytecode programs
 *
 * ldc: push item from the runtime constant pool
 * iadd: add int (requires value1: int and value2: int on the stack)
 * iconst <arg1> | push int constant
 *
 * Needed for fac:
 * iload<n> | Load local var
 * ifne
 * iconst <arg1> | push int constant
 * ireturn
 * istore<n> | store int in local variable
 * if_icmpgt
 * imul
 * iinc<local var, amount> | increment local var by amount
 * goto
 *
 *
 * Use jasmin to write test bytecode file. Apparently constant folding can't be disabled in javac.
 */

/*
typedef enum {
  ldc  = 18,
  iadd = 96,
  return_void = 177,
  iconst,
  iload,
  ifne,
  ireturn,
  istore,
  if_icmpgt,
  imul,
  iinc,
  goto_instr,
} instruction_type;

typedef struct  {
  instruction_type type;
  union {
   u4 arg1;
  } args;
} instruction;

instruction fetch_instruction(u1* bytecode, int *out_num_bytes_consumed);

void execute(code_attribute method_code);
*/

#endif
