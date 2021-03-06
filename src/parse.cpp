#include "class_file.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

void pretty_print_constant_tag(u8 tag) {
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
    default: ; // printf("Unknown constant tag %d\n", tag);
  }
}

attribute_type parse_attribute_type(char *unicode_name, int length) {
  if (strncmp(unicode_name, "Code"      , length) == 0) return Code_attribute;
  if (strncmp(unicode_name, "Constant"  , length) == 0) return ConstantValue_attribute;
  if (strncmp(unicode_name, "SourceFile", length) == 0) return SourceFile_attribute;
  if (strncmp(unicode_name, "LineNumberTable", length) == 0) return LineNumberTable_attribute;
  if (strncmp(unicode_name, "StackMapTable", length) == 0) return StackMapTable_attribute;
  return Unknown_attribute;
}

attribute_info parse_attribute_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */) {
  attribute_info info = (attribute_info) { };
  u16 attribute_name_index = __builtin_bswap16(*((u16 *) data));
  info.attribute_name = &class_file->constant_pool[attribute_name_index-1].as.utf8_info;
  info.attribute_length = __builtin_bswap32(*((u32 *) (data + 2)));

  info.type = parse_attribute_type((char *)info.attribute_name->bytes, info.attribute_name->length);

#ifdef DEBUG
  assert(class_file->constant_pool[attribute_name_index-1].tag == CONSTANT_Utf8);
#endif


  int offset = 6;

  if (info.attribute_length == 0) {
    *out_byte_size = offset;
    return info;
  }

  switch(info.type) {
  case Code_attribute:
  {
    code_attribute *code_attr = (code_attribute *)malloc(sizeof(code_attribute));
    code_attr->max_stack = __builtin_bswap16(*((u16 *)(data + offset))); offset += 2;
    code_attr->max_locals = __builtin_bswap16(*((u16 *)(data + offset))); offset += 2;
    code_attr->code_length = __builtin_bswap32(*((u32 *)(data + offset))); offset += 4;

    code_attr->code = (u8 *)malloc(code_attr->code_length);
    memcpy(code_attr->code, data+offset, code_attr->code_length);
    offset += code_attr->code_length;

    code_attr->exception_table_length = __builtin_bswap16(*((u16 *)(data + offset))); offset += 2;
    if (code_attr->exception_table_length > 0) {
      int exception_table_bytes = code_attr->exception_table_length * sizeof(exception_table_entry);
      code_attr->exception_table = (exception_table_entry *)malloc(exception_table_bytes);
      // TODO(Noah): Endianness Problems
      memcpy(code_attr->exception_table, data+offset, exception_table_bytes);
      offset += exception_table_bytes;
    } else {
      code_attr->exception_table = NULL;
    }

    code_attr->attributes_count = __builtin_bswap16(*((u16 *)(data + offset))); offset += 2;
    if (code_attr->attributes_count > 0) {
      code_attr->attributes = (attribute_info *)malloc(code_attr->attributes_count * sizeof(attribute_info));
      for (int i = 0; i < code_attr->attributes_count; i++) {
        int attribute_size = 0;
        code_attr->attributes[i] = parse_attribute_info(class_file, data+offset, &attribute_size);
        offset += attribute_size;
      }
    } else {
      code_attr->attributes = NULL;
    }

    info.as.code_attr = code_attr;
  } break;
  case ConstantValue_attribute:
  {
    info.as.constantvalue_index = __builtin_bswap16(*((u16 *)(data+offset)));
    offset += 2;
  } break;
  case SourceFile_attribute:
  {
    u16 sourcefile_index = __builtin_bswap16(*((u16 *)(data+offset)));
    info.as.sourcefile = &class_file->constant_pool[sourcefile_index-1].as.utf8_info;
    offset += 2;
  } break;
  case LineNumberTable_attribute: // TODO
  case StackMapTable_attribute: // TODO
  case Unknown_attribute:
  default:
    // printf("Unhandled attribute with name %.*s\n", info.attribute_name->length, info.attribute_name->bytes);
    info.as.bytes = (u8 *)malloc(info.attribute_length);
    memcpy(info.as.bytes, data+offset, info.attribute_length);
    offset += info.attribute_length;
  }

#ifdef DEBUG
  assert(offset == info.attribute_length + 6);
#endif

  *out_byte_size = offset;

  return info;
}

method_info parse_method_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */) {
  int offset = 0;
  method_info info = (method_info) { };

  info.access_flags = __builtin_bswap16(*((u16 *) data));

  u16 name_index = __builtin_bswap16(*(((u16 *) data)+1));
  info.name = &class_file->constant_pool[name_index-1].as.utf8_info;

  u16 descriptor_index = __builtin_bswap16(*(((u16 *) data)+2));
  info.descriptor = &class_file->constant_pool[descriptor_index-1].as.utf8_info;

  info.attributes_count = __builtin_bswap16(*(((u16 *) data)+3));
  offset = 8;

  if (info.attributes_count == 0) {
    *out_byte_size = offset;
    info.attributes = NULL;
    return info;
  }

  info.attributes = (attribute_info *)malloc(sizeof(attribute_info) * info.attributes_count);
  for (int i = 0; i < info.attributes_count; i++) {
    int attribute_size = 0;
    info.attributes[i] = parse_attribute_info(class_file, data+offset, &attribute_size);
    offset += attribute_size;
  }

  *out_byte_size = offset;
  return info;
}


field_info parse_field_info(ClassFile *class_file, u8 *data, int *out_byte_size /* How many bytes were parsed */) {
  field_info info = (field_info) { };
  int offset = 0;

  info.access_flags = __builtin_bswap16(*((u16 *) data));
  u16 name_index = __builtin_bswap16(*(((u16 *) data)+1));
  info.name = &class_file->constant_pool[name_index-1].as.utf8_info;

  u16 descriptor_index = __builtin_bswap16(*(((u16 *) data)+2));
  info.descriptor = &class_file->constant_pool[descriptor_index-1].as.utf8_info;
  info.attributes_count = __builtin_bswap16(*(((u16 *) data)+3));
  offset += 8;

  if (info.attributes_count == 0) {
    info.attributes = NULL;
    *out_byte_size = offset;
    return info;
  }

  info.attributes = (attribute_info *)malloc(sizeof(attribute_info) * info.attributes_count);

  for (int i = 0; i < info.attributes_count; i++) {
    int attribute_size = 0;
    info.attributes[i] = parse_attribute_info(class_file, data + offset, &attribute_size);

    offset += attribute_size;
  }

  *out_byte_size = offset;

  return info;
}

cp_info parse_cp_info(cp_info *constant_pool, u8 *data, int *out_byte_size /* How many bytes were parsed */) {
  cp_info info = (cp_info) { };
  info.tag = data[0];

  switch (info.tag) {
  case CONSTANT_Methodref:
  {
    u16 class_index = __builtin_bswap16(*((u16 *)(data+1)));
    info.as.methodref_info.class_info = &constant_pool[class_index-1].as.class_info;
    u16 name_and_type_index = __builtin_bswap16(*((u16 *)(data+3)));
    info.as.methodref_info.name_and_type = &constant_pool[name_and_type_index-1].as.name_and_type_info;
    *out_byte_size = 5;
  } break;
  case CONSTANT_Class:
  {
    u16 name_index = __builtin_bswap16(*((u16 *)(data+1)));
    info.as.class_info.name = &constant_pool[name_index-1].as.utf8_info;
    *out_byte_size = 3;
  } break;
  case CONSTANT_NameAndType:
  {
    u16 name_index = __builtin_bswap16(*((u16 *)(data+1)));
    info.as.name_and_type_info.name = &constant_pool[name_index-1].as.utf8_info;
    u16 descriptor_index = __builtin_bswap16(*((u16 *)(data+3)));
    info.as.name_and_type_info.descriptor = &constant_pool[descriptor_index-1].as.utf8_info;
    *out_byte_size = 5;
  } break;
  case CONSTANT_Utf8:
  {
    // TODO: Length 0? Don't malloc in this case
    info.as.utf8_info.length = __builtin_bswap16(*((u16 *)(data+1)));
    info.as.utf8_info.bytes = (u8 *)malloc(info.as.utf8_info.length);
    memcpy(info.as.utf8_info.bytes, data+3, info.as.utf8_info.length);
    *out_byte_size = 3 + info.as.utf8_info.length;
  } break;
  case CONSTANT_Fieldref:
  {
    u16 class_index = __builtin_bswap16(*((u16 *)(data+1)));
    info.as.fieldref_info.class_info = &constant_pool[class_index-1].as.class_info;
    u16 name_and_type_index = __builtin_bswap16(*((u16 *)(data+3)));
    info.as.fieldref_info.name_and_type = &constant_pool[name_and_type_index-1].as.name_and_type_info;
    *out_byte_size = 5;
  } break;
  case CONSTANT_String:
  {
    u16 string_index = __builtin_bswap16(*((u16 *)(data+1)));
    info.as.string_info.string = &constant_pool[string_index-1].as.utf8_info;
    *out_byte_size = 3;
  } break;
  case CONSTANT_Double:
  {
    // TODO: This does not work
    u64 high_bytes = __builtin_bswap32(*((u32 *)(data + 1)));
    u64 low_bytes = __builtin_bswap32(*((u32 *)(data + 5)));
    info.as.double_value = (high_bytes << 32)| low_bytes;
    *out_byte_size = 9;
  } break;
  case CONSTANT_Integer:
  {
    info.as.integer_value = __builtin_bswap32(*((u32 *)(data + 1)));
    *out_byte_size = 5;
  } break;
  case CONSTANT_InvokeDynamic:
  {
    info.as.invoke_dynamic_info.bootstrap_method_attr_index = __builtin_bswap16(*((u16 *)(data + 1)));
    u16 name_and_type_index = __builtin_bswap16(*((u16 *)(data + 3)));
    info.as.invoke_dynamic_info.name_and_type = &constant_pool[name_and_type_index-1].as.name_and_type_info;
    *out_byte_size = 5;
  } break;
  case CONSTANT_MethodHandle:
  {
    info.as.method_handle_info.reference_kind = *(data + 1);
    info.as.method_handle_info.reference_index = *((u16 *)data + 2);
    *out_byte_size = 4;
  } break;
  default:
  {
    printf("Unhandled cp_info type %u\n", info.tag);
    assert(false);
  }
  }
  return info;
}

ClassFile *parse_class_file(char *filename) {
  // mmap the whole file for now
  int fd = open(filename, O_RDONLY);
  if (fd == -1) return NULL;

  struct stat stat;
  fstat(fd, &stat);

  u8* data_start = (u8 *)mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
  u8* data = data_start;

  ClassFile *class_file = (ClassFile *)malloc(sizeof(ClassFile));

  class_file->magic               = __builtin_bswap32(*((u32 *)data)); data += 4;
  class_file->minor_version       = __builtin_bswap16(*((u16 *)data)); data += 2;
  class_file->major_version       = __builtin_bswap16(*((u16 *)data)); data += 2;
  class_file->constant_pool_count = __builtin_bswap16(*((u16 *)data)); data += 2;

  class_file->constant_pool = (cp_info *)malloc(sizeof(cp_info)*(class_file->constant_pool_count-1));
  int byte_size = 0;
  for (int i = 0; i < class_file->constant_pool_count-1; i++) {
    class_file->constant_pool[i] = parse_cp_info(class_file->constant_pool, data, &byte_size);
    if (class_file->constant_pool[i].tag == CONSTANT_Long
        || class_file->constant_pool[i].tag == CONSTANT_Double) {
      // NOTE(Noah): Indexing is still right. We just have a uninitialized field
      i+=1; // Long and Double constants take up 8 bytes and 2 slots in the constant table
      // TODO(Noah): Maybe initialize the field to something that tells us "this slot is not used"
    }
    data += byte_size;
  }

  class_file->access_flags = __builtin_bswap16(*((u16 *)(data))); data += 2;
  class_file->this_class = __builtin_bswap16(*((u16 *)(data))); data += 2;
  class_file->super_class = __builtin_bswap16(*((u16 *)(data))); data += 2;
  class_file->interfaces_count = __builtin_bswap16(*((u16 *)(data))); data += 2;
  if (class_file->interfaces_count > 0) {
    class_file->interfaces = (u16 *)malloc(class_file->interfaces_count * sizeof(u16));
    // TODO(Noah): This doesn't work because we have to convert the endianness
    memcpy(class_file->interfaces, data, class_file->interfaces_count * sizeof(u16));
    data += class_file->interfaces_count * sizeof(u16);
  } else {
    class_file->interfaces = NULL;
  }
  class_file->fields_count = __builtin_bswap16(*((u16 *)data)); data += 2;
  if (class_file->fields_count > 0) {
    class_file->fields = (field_info *)malloc(class_file->fields_count * sizeof(field_info));
    for (int i = 0; i < class_file->fields_count; i++) {
      int field_info_size = 0;
      class_file->fields[i] = parse_field_info(class_file, data, &field_info_size);
      data += field_info_size;
    }
  } else {
    class_file->fields = NULL;
  }

  class_file->methods_count = __builtin_bswap16(*((u16 *)(data))); data += 2;
  if (class_file->methods_count == 0) {
    class_file->methods = NULL;
  } else {
    class_file->methods = (method_info *)malloc(sizeof(method_info) * class_file->methods_count);
    for (int i = 0; i < class_file->methods_count; i++) {
      int method_info_size = 0;
      class_file->methods[i] = parse_method_info(class_file, data, &method_info_size);
      data += method_info_size;
    }
  }

  class_file->attributes_count = __builtin_bswap16(*((u16 *)(data))); data += 2;
  if (class_file->attributes_count == 0) {
    class_file->attributes = NULL;
  } else {
    class_file->attributes = (attribute_info *)malloc(sizeof(attribute_info) * class_file->attributes_count);
    for (int i = 0; i < class_file->attributes_count; i++) {
      int attribute_info_size = 0;
      class_file->attributes[i] = parse_attribute_info(class_file, data, &attribute_info_size);
      data += attribute_info_size;
    }
  }

  assert((data - data_start) == stat.st_size); // bytes read == file size

  munmap(data_start, stat.st_size);
  close(fd); // close after the mmap?
  
  return class_file;
}

void free_class_file(ClassFile *class_file) {
  // TODO
}
