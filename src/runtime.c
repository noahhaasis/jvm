#include "runtime.h"

#include "class_file.h"
#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

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

code_attribute *find_code(ClassFile *class_file, method_info method_info) {
  for (int i = 0; i < method_info.attributes_count; i++) {
    attribute_info info = method_info.attributes[i];
    cp_info name_constant = class_file->constant_pool[info.attribute_name_index-1];

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

method_info find_method(ClassFile *class_file, char *name, u32 name_length) {
  for (int i = 0; i < class_file->methods_count; i++) {
    method_info info = class_file->methods[i];
    cp_info name_constant = class_file->constant_pool[info.name_index - 1];

    if (strncmp(
          name,
          (const char *)name_constant.as.utf8_info.bytes,
          MIN(name_constant.as.utf8_info.length, name_length)) == 0) {
      return info;
    }
  }
  printf("Failed to find method \"%s\"", name);
  assert(0);
  return (method_info){};
}

/* TODO(noah):
 * Instead of having a seperate frame stack just push
 * the pointers on the stack and pop them.
 */

typedef struct {
  u8 *pc;
  u32 *stack;
  u32 *sp;
  u32 *locals;
} frame;

frame create_frame_from_code_attribute(code_attribute code_attr) {
  u32 *stack =
    code_attr.max_stack > 0
    ? malloc(code_attr.max_stack * sizeof(u32))
    : NULL;
  return (frame) {
    .pc = code_attr.code,
    .stack = stack,
    .sp = stack,
    .locals =
      code_attr.max_locals > 0
      ? malloc(code_attr.max_locals * sizeof(u32))
      : NULL,
  };
}

void free_frame(frame frame) {
  if (frame.stack)  free(frame.stack);
  if (frame.locals) free(frame.locals);
}

void execute(ClassFile *class_file, code_attribute entry_method) {
  frame *frames = NULL;
  /* Current frame */
  frame f = create_frame_from_code_attribute(entry_method);

  while (true) { // TODO(noah): Prevent overflows here
    bool skip_pc_increment = false;
    i32 offset = 1;
    u8 bytecode = *f.pc;

    switch(bytecode) {
    /* iconst_<n> */
    case 2: case 3: case 4: case 5:
    case 6: case 7: case 8:
    {
      u8 n = bytecode - 3;
      *f.sp = n;
      f.sp += 1;
    } break;
    /* iload_<n> */
    case 26: case 27: case 28: case 29:
    {
      u8 local_var_index = bytecode - 26;
      *f.sp = f.locals[local_var_index];
      f.sp += 1;
    } break;
    /* istore_<n> */
    case 59: case 60: case 61: case 62:
    {
      u8 local_var_index = bytecode - 59;
      f.sp -= 1;
      f.locals[local_var_index] = *f.sp;
    } break;
    case pop: { f.sp -= 1; } break;
    // TODO: Macro ifCOND(op) ...
    case ifeq:
    {
      u8 branchbyte1 = *(f.pc+offset); offset += 1;
      u8 branchbyte2 = *(f.pc+offset); offset += 1;

      f.sp -= 1;
      u32 value = *f.sp;

      if (value == 0) { // Branch succeeds
        offset = (i16)((branchbyte1 << 8) | branchbyte2);
      }
    } break;
    case ifne:
    {
      u8 branchbyte1 = *(f.pc+offset); offset += 1;
      u8 branchbyte2 = *(f.pc+offset); offset += 1;

      f.sp -= 1;
      u32 value = *f.sp;

      if (value != 0) { // Branch succeeds
        offset = (i16)((branchbyte1 << 8) | branchbyte2);
      }
    } break;
    case if_icmpgt:
    {
      u8 branchbyte1 = *(f.pc+offset); offset += 1;
      u8 branchbyte2 = *(f.pc+offset); offset += 1;

      f.sp -= 1; u32 value2 = *f.sp;
      f.sp -= 1; u32 value1 = *f.sp;

      if (value1 > value2) { // Branch succeeds
        offset = (i16) ((branchbyte1 << 8) | branchbyte2);
      }
    } break;
    case if_icmpne:
    {
      u8 branchbyte1 = *(f.pc+offset); offset += 1;
      u8 branchbyte2 = *(f.pc+offset); offset += 1;

      f.sp -= 1; u32 value2 = *f.sp;
      f.sp -= 1; u32 value1 = *f.sp;

      if (value1 != value2) { // Branch succeeds
        offset = (i16) ((branchbyte1 << 8) | branchbyte2);
      }
    } break;
    case ireturn:
    {
      f.sp -= 1;
      u32 value = *f.sp;
      printf("Returning int %d\n", value);
      if (sb_is_empty(frames)) {
        goto exit;
      }
#ifdef DEBUG
      assert(f.sp == f.stack);
#endif
      free_frame(f);
      f = frames[sb_length(frames)-1];
      sb_pop(frames);

      // push the return value
      *f.sp = value;
      f.sp++;
      skip_pc_increment = true;
    } break;
    case return_void:
    {
      printf("Returning void\n");
      if (sb_is_empty(frames)) {
        goto exit;
      }
#ifdef DEBUG
      assert(f.sp == f.stack);
#endif
      // TODO: Use sb_pop and make it actually return the poped value
      free_frame(f);
      f = frames[sb_length(frames)];
      sb_pop(frames);
      skip_pc_increment = true;
    } break;
    // TODO: Refactor into macro BINOP
    case imul:
    {
      f.sp -= 1; u32 value2 = *f.sp;
      u32 value1 = *(f.sp-1);
      *(f.sp-1) = (int)value1 * (int)value2;
    } break;
    case iadd:
    {
      f.sp -= 1; u32 value2 = *f.sp;
      u32 value1 = *(f.sp-1);
      *(f.sp-1) = (int)value1 + (int)value2;
    } break;
    case isub:
    {
      f.sp -= 1; u32 value2 = *f.sp;
      u32 value1 = *(f.sp-1);
      *(f.sp-1) = (int)value1 - (int)value2;
    } break;
    case iinc:
    {
      u8 index = *(f.pc + offset); offset += 1;
      i8 const_value = *(f.pc + offset); offset += 1;

      f.locals[index] += const_value;
    } break;
    case invokestatic:
    {
      u8 indexbyte1 = *(f.pc+offset); offset += 1;
      u8 indexbyte2 = *(f.pc+offset); offset += 1;
      u16 index = (indexbyte1 << 8) | indexbyte2;

      u16 name_and_type_index =
        class_file->constant_pool[index-1].as.methodref_info.name_and_type_index;
      u16 name_index =
        class_file->constant_pool[name_and_type_index-1].as.name_and_type_info.name_index;
      cp_info method_name_constant = class_file->constant_pool[name_index-1];

      method_info method_info = find_method(
          class_file,
          (char *)method_name_constant.as.utf8_info.bytes,
          method_name_constant.as.utf8_info.length);
      code_attribute *method_code = find_code(class_file, method_info);

      // Add the offset now so that the function returns to the next instruction
      // of this frame
      f.pc += offset;

      // Pop all args from the stack and store them in locals[] of the new frame
      cp_info method_descriptor_string =
        class_file->constant_pool[method_info.descriptor_index-1];
      method_descriptor method_descriptor =
        parse_method_descriptor(
            (char *)method_descriptor_string.as.utf8_info.bytes,
            method_descriptor_string.as.utf8_info.length);

      frame new_frame = create_frame_from_code_attribute(*method_code);

      // Copy all args from the callers stack to the callees local vars
      memcpy(
          new_frame.locals,
          ((u8 *) f.sp) - method_descriptor.all_params_byte_count,
          method_descriptor.all_params_byte_count);
      // Pop all args
      f.sp = ((u8 *) f.sp) - method_descriptor.all_params_byte_count;

      sb_push(frames, f);
      f = new_frame;

      skip_pc_increment = true;
    } break;
    case goto_instr:
    {
      u8 branchbyte1 = *(f.pc+offset); offset += 1;
      u8 branchbyte2 = *(f.pc+offset); offset += 1;

      offset = (i16)((branchbyte1 << 8) | branchbyte2);
    } break;
    default: printf("Unhandled instruction with opcode %d\n", bytecode);
    }

    if (!skip_pc_increment) { f.pc += offset; }
  }

  printf("Returning without a \"return\" statement\n");

exit:
#ifdef DEBUG
  assert(f.sp == f.stack);
#endif
  free_frame(f);
}
