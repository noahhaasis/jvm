#include "runtime.h"

#include "buffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

method_info find_method(ClassFile *class_file, char *name, u32 name_length) {
  for (int i = 0; i < class_file->methods_count; i++) {
    method_info info = class_file->methods[i];
    cp_info name_constant = class_file->constant_pool[info.name_index - 1];

    u32 length = name_constant.as.utf8_info.length;
    if (length == name_length &&
        strncmp(
          name,
          (const char *)name_constant.as.utf8_info.bytes,
          name_length) == 0) {
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
} Frame;

Frame create_frame_from_code_attribute(code_attribute code_attr) {
  u32 *stack =
    code_attr.max_stack > 0
    ? malloc(code_attr.max_stack * sizeof(u32))
    : NULL;
  return (Frame) {
    .pc = code_attr.code,
    .stack = stack,
    .sp = stack,
    .locals =
      code_attr.max_locals > 0
      ? malloc(code_attr.max_locals * sizeof(u32))
      : NULL,
  };
}

void free_frame(Frame frame) {
  if (frame.stack)  free(frame.stack);
  if (frame.locals) free(frame.locals);
}

void execute_main(char *filename) {
  ClassLoader class_loader = ClassLoader_create();

  Class *main_class = load_class_from_file(
      class_loader,
      "Main", 4,// TODO: Classname
      filename, strlen(filename)
    );

  // call <clinit>
  Method *clinit = HashMap_get(main_class->method_map, "<clinit>", strlen("<clinit>"));
  execute(class_loader, clinit);


  Method *main_method = HashMap_get(main_class->method_map, "main", strlen("main")); // TODO
  execute(class_loader, main_method);
  // ClassLoader_destroy(&);
}

// Inline?
intern inline u32 f_pop(Frame *f) {
  f->sp -= 1;
  return *f->sp;
}

intern inline void f_push(Frame *f, u32 value) {
  *f->sp = value;
  f->sp += 1;
}

intern Class *load_class_and_field_name_from_field_ref(
    ClassLoader class_loader, Method *method, u16 index,
    cp_info *out_field_name) {
      // Field resolution
    cp_info field_ref_info = method->constant_pool[index-1];
    cp_info class_info = method->constant_pool[
      field_ref_info.as.field_ref_info.class_index-1];
    cp_info class_name = method->constant_pool[
      class_info.as.class_info.name_index-1];
    Class *class = get_class(
        class_loader,
        (char *)class_name.as.utf8_info.bytes, 
        class_name.as.utf8_info.length);
    if (!class) {
      // load and initialize class
      load_class(
        class_loader, 
        (char *)class_name.as.utf8_info.bytes, 
        class_name.as.utf8_info.length);
      // TODO: Initialize
    }

    cp_info name_and_type = method->constant_pool[
      field_ref_info.as.field_ref_info.name_and_type_index-1];
    *out_field_name = method->constant_pool[
      name_and_type.as.name_and_type_info.name_index-1];

    return class;
}

void execute(ClassLoader class_loader, Method *method) {
  assert(method);

  // TODO(noah): Store the current class?
  Frame *frames = NULL;
  /* Current frame */
  Frame f = create_frame_from_code_attribute(*method->code_attr);

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
    case bipush:
    {
      u8 n = *(f.pc+offset); offset += 1;
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
      f = frames[sb_length(frames) - 1];
      sb_pop(frames);
      skip_pc_increment = true;
    } break;
    case getstatic:
    {
      u8 indexbyte1 = *(f.pc+offset); offset += 1;
      u8 indexbyte2 = *(f.pc+offset); offset += 1;
      u16 index = (indexbyte1 << 8) | indexbyte2;

      cp_info field_name;
      Class *class = load_class_and_field_name_from_field_ref(
          class_loader, method, index, &field_name);

      u32 value = get_static(
          class,
          (char *)field_name.as.utf8_info.bytes, 
          field_name.as.utf8_info.length);
    
      f_push(&f, value);

    } break;
    case putstatic:
    {
      u8 indexbyte1 = *(f.pc+offset); offset += 1;
      u8 indexbyte2 = *(f.pc+offset); offset += 1;
      u16 index = (indexbyte1 << 8) | indexbyte2;

      u32 value = f_pop(&f);

      cp_info field_name;

      Class *class = load_class_and_field_name_from_field_ref(
          class_loader, method, index, &field_name);

      set_static(
          class,
          (char *)field_name.as.utf8_info.bytes,
          field_name.as.utf8_info.length,
          value);
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
        method->constant_pool[index-1].as.methodref_info.name_and_type_index;
      u16 name_index =
        method->constant_pool[name_and_type_index-1].as.name_and_type_info.name_index;
      cp_info method_name_constant = method->constant_pool[name_index-1];

      u16 class_index =
        method->constant_pool[index-1].as.methodref_info.class_index;
      u16 class_name_index =
        method->constant_pool[class_index-1].as.class_info.name_index;
      cp_info class_name_constant = method->constant_pool[class_name_index-1];

      Class *class = get_class(
          class_loader,
          (char *)class_name_constant.as.utf8_info.bytes,
          class_name_constant.as.utf8_info.length);
      if (!class) {
        class = load_class(
            class_loader,
            (char *)class_name_constant.as.utf8_info.bytes,
            class_name_constant.as.utf8_info.length);
        // TODO: call <clinit>
      }

      Method *method = HashMap_get(
          class->method_map,
          (char *)method_name_constant.as.utf8_info.bytes,
          method_name_constant.as.utf8_info.length);

      code_attribute *method_code = method->code_attr;

      // Add the offset now so that the function returns to the next instruction
      // of this frame
      f.pc += offset;

      // Pop all args from the stack and store them in locals[] of the new frame
      method_descriptor method_descriptor = method->descriptor;

      Frame new_frame = create_frame_from_code_attribute(*method_code);

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
    case invokespecial:
    {
      assert(false);
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
