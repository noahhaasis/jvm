#ifndef CLASSFILE_H
#define CLASSFILE_H

#include "common.h"
#include "Vector.h"

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

struct Utf8Info {
  u16 length;
  u8* bytes;
};

struct NameAndTypeInfo {
  Utf8Info *name;
  Utf8Info *descriptor;
};

struct ClassInfo {
  Utf8Info *name;
};

struct FieldrefInfo {
  ClassInfo *class_info;
  NameAndTypeInfo *name_and_type;
};

struct MethodrefInfo {
  ClassInfo *class_info;
  NameAndTypeInfo *name_and_type;
};

struct InterfaceMethodrefInfo {
  ClassInfo *class_info;
  NameAndTypeInfo *name_and_type;
};

struct StringInfo {
  Utf8Info *string;
};

struct IntegerInfo {
  u32 bytes;
};

struct MethodHandleInfo {
  u8 reference_kind;
  u16 reference_index;
};

struct MethodTypeInfo {
  Utf8Info *descriptor;
};

struct InvokeDynamicInfo {
  u16 bootstrap_method_attr_index;
  NameAndTypeInfo *name_and_type;
};

// TODO: Store the tag in a seperate list.
// The tag is only needed for verification
struct cp_info {
    u8 tag;
    union {
      ClassInfo class_info;
      FieldrefInfo fieldref_info;
      MethodrefInfo methodref_info;
      InterfaceMethodrefInfo interface_methodref_info;
      StringInfo string_info;
      IntegerInfo integer_info;
      float  float_value;
      i32    integer_value;
      i64    long_value;
      double double_value;
      NameAndTypeInfo name_and_type_info;
      Utf8Info utf8_info;
      MethodHandleInfo method_handle_info;
      MethodTypeInfo method_type_info;
      InvokeDynamicInfo invoke_dynamic_info;
    } as;
    // TODO: remove "as"
    /*u8 info[]*/;
};

struct exception_table_entry {
  u16 start_pc;
  u16 end_pc;
  u16 handler_pc;
  u16 catch_type;
};

enum instruction_type {
  iconst        = 2,  /* 2 - 8 ; Push iconst_<n>*/
  bipush        = 16,
  sipush        = 17,
  ldc           = 18,
  iload         = 21,
  aload         = 25,
  iload_n       = 26, /* 26 - 29 */
  aload_n       = 42, /* 42 - 45 */
  istore        = 54,
  istore_n      = 59, /* 59 - 62 */
  astore_n      = 75, /* 75 - 78 */
  pop           = 87,
  dup_instr     = 89, // damn you <unistd.h> for taking that beautiful name
  iadd          = 96,
  isub          = 100,
  imul          = 104,
  iinc          = 132,
  ifeq          = 153,
  ifne          = 154,
  if_icmpne     = 160,
  if_icmpge     = 162,
  if_icmpgt     = 163,
  goto_instr    = 167,
  ireturn       = 172,
  return_void   = 177,
  getstatic     = 178,
  putstatic     = 179,
  getfield      = 180,
  putfield      = 181,
  invokevirtual = 182,
  invokespecial = 183,
  invokestatic  = 184,
  new_instr     = 187,
  undefined,
};

struct attribute_info;

struct code_attribute {
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
};

enum attribute_type {
  SourceFile_attribute,
  ConstantValue_attribute,
  Code_attribute,
  LineNumberTable_attribute,
  StackMapTable_attribute,
  Unknown_attribute
};

struct attribute_info {
    Utf8Info *attribute_name;
    u32 attribute_length;
    attribute_type type;
    union {
      /* SourceFile_attribute */
      Utf8Info *sourcefile;

      /* ConstantValue_attribute */
      u16 constantvalue_index;

      /* Code_attribute */
      code_attribute *code_attr;

      /* other */
      u8* bytes;
    } as;
    // u8 *info;
    /* u8 info[attribute_length]; */
};

struct method_info {
    u16             access_flags;
    Utf8Info *      name;
    Utf8Info *      descriptor;
    u16             attributes_count;
    attribute_info *attributes;
    /* attribute_info attributes[attributes_count]; */
};

enum primitive_type {
  int_t = 1,
  double_t,
};

enum return_descriptor {
  void_t = 0,
  // ... primitive_type
};

struct method_descriptor {
  Vector<primitive_type> parameter_types;
  return_descriptor return_type;
};

struct field_info {
    u16             access_flags;
    Utf8Info *      name;
    Utf8Info *      descriptor;
    u16             attributes_count;
    attribute_info *attributes;
    /* attribute_info attributes[attributes_count]; */
};

#define FIELD_ACC_STATIC 0x0008u

struct ClassFile {
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
};

attribute_type parse_attribute_type(char *unicode_name, int length);

attribute_info parse_attribute_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */);

method_info parse_method_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */);

field_info parse_field_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */);

cp_info parse_cp_info(u8 *data, int *out_byte_size /* How many bytes were parsed */);

ClassFile *parse_class_file(char *filename);

void free_class_file(ClassFile *class_file);

#endif
