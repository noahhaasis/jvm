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
  u64 *stack;
  u64 *sp;
  u64 *locals;
} Frame;

Frame create_frame_from_code_attribute(code_attribute code_attr) {
  u64 *stack =
    code_attr.max_stack > 0
    ? malloc(code_attr.max_stack * sizeof(u64))
    : NULL;
  return (Frame) {
    .pc = code_attr.code,
    .stack = stack,
    .sp = stack,
    .locals =
      code_attr.max_locals > 0
      ? malloc(code_attr.max_locals * sizeof(u64))
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
      (String) { .bytes = "Main", .length = 4 },// TODO: Classname
      (String) { .bytes = filename, .length = strlen(filename) }
    );

  // call <clinit>
  Method *clinit = HashMap_get(
      main_class->method_map,
      (String) {
        .bytes = "<clinit>",
        .length = strlen("<clinit>")
      });
  execute(class_loader, clinit);


  Method *main_method = HashMap_get(
      main_class->method_map, 
      (String) {
        .bytes = "main",
        .length = strlen("main")
      }); // TODO
  execute(class_loader, main_method);
  // ClassLoader_destroy(&);
}

intern inline u64 f_pop(Frame *f) {
  f->sp -= 1;
  return *f->sp;
}

intern inline void f_push(Frame *f, u64 value) {
  *f->sp = value;
  f->sp += 1;
}

intern Class *load_class_and_field_name_from_field_ref(
    ClassLoader class_loader, Method *method, u16 index,
    cp_info *out_field_name) {
      // Field resolution
    cp_info fieldref_info = method->constant_pool[index-1];
    cp_info class_info = method->constant_pool[
      fieldref_info.as.fieldref_info.class_index-1];
    cp_info class_name = method->constant_pool[
      class_info.as.class_info.name_index-1];
    Class *class = get_class(
        class_loader,
        (String) {
          .bytes = (char *)class_name.as.utf8_info.bytes, 
          .length = class_name.as.utf8_info.length
        });
    if (!class) {
      // load and initialize class
      load_class(
        class_loader, 
        (String) {
          .bytes = (char *)class_name.as.utf8_info.bytes, 
          .length = class_name.as.utf8_info.length
        });
      // TODO: Initialize
    }

    cp_info name_and_type = method->constant_pool[
      fieldref_info.as.fieldref_info.name_and_type_index-1];
    *out_field_name = method->constant_pool[
      name_and_type.as.name_and_type_info.name_index-1];

    return class;
}

Class *load_class_from_constant_pool(ClassLoader class_loader, cp_info *constant_pool, u16 index) {
  cp_info class_info = constant_pool[index-1];
  cp_info class_name = constant_pool[class_info.as.class_info.name_index];
  
  Class *class = get_class(
      class_loader,
      (String) {
        .bytes = (char *)class_name.as.utf8_info.bytes,
        .length = class_name.as.utf8_info.length
      });
  if (!class) {
    class = load_class( 
      class_loader,
      (String) {
        .bytes = (char *)class_name.as.utf8_info.bytes,
        .length = class_name.as.utf8_info.length
      });
  }

  return class;
}

// Well this is just obviously bad :^)
typedef struct {
  Class *class;
  // HashMap<string field, u32 value>
  HashMap *fields;
} Object;

Object *Object_create(Class *class) {
  Object *obj = malloc(sizeof(Object));
  obj->class = class;
  obj->fields = HashMap_create();
  return obj;
}

void execute(ClassLoader class_loader, Method *method) {
  assert(method);

  // TODO(noah): Store the current class?
  Frame *frames = NULL;
  /* Current frame */
  Frame f = create_frame_from_code_attribute(*method->code_attr);

  while (true) { // TODO(noah): Prevent overflows here
    u8 bytecode = *f.pc++;

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
      u8 n = *(f.pc++);
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
    /* aload_<n> */
    case 42: case 43: case 44: case 45:
    {
      u16 local_var_index = bytecode - 42;
      f_push(&f, f.locals[local_var_index]);
    } break;
    /* istore_<n> */
    case 59: case 60: case 61: case 62:
    {
      u8 local_var_index = bytecode - 59;
      f.sp -= 1;
      f.locals[local_var_index] = *f.sp;
    } break;
    /* astore_<n> */
    case 75: case 76: case 77: case 78:
    {
      u16 local_var_index = bytecode - 75;
      u64 reference = f_pop(&f);
      f.locals[local_var_index] = reference;
    } break;
    case pop: { f.sp -= 1; } break;
    case dup_instr: { f_push(&f, *(f.sp-1)); } break;
    // TODO: Macro ifCOND(op) ...
    case ifeq:
    {
      u8 branchbyte1 = *(f.pc++);
      u8 branchbyte2 = *(f.pc++);

      f.sp -= 1;
      u32 value = *f.sp;

      if (value == 0) { // Branch succeeds
        int offset = (i16)((branchbyte1 << 8) | branchbyte2);
        f.pc += offset - 3;
      }
    } break;
    case ifne:
    {
      u8 branchbyte1 = *(f.pc++);
      u8 branchbyte2 = *(f.pc++);

      f.sp -= 1;
      u32 value = *f.sp;

      if (value != 0) { // Branch succeeds
        int offset = (i16)((branchbyte1 << 8) | branchbyte2);
        f.pc += offset - 3;
      }
    } break;
    case if_icmpgt:
    {
      u8 branchbyte1 = *(f.pc++);
      u8 branchbyte2 = *(f.pc++);

      f.sp -= 1; u32 value2 = *f.sp;
      f.sp -= 1; u32 value1 = *f.sp;

      if (value1 > value2) { // Branch succeeds
        int offset = (i16) ((branchbyte1 << 8) | branchbyte2);
        f.pc += offset - 3;
      }
    } break;
    case if_icmpne:
    {
      u8 branchbyte1 = *(f.pc++);
      u8 branchbyte2 = *(f.pc++);

      f.sp -= 1; u32 value2 = *f.sp;
      f.sp -= 1; u32 value1 = *f.sp;

      if (value1 != value2) { // Branch succeeds
        int offset = (i16) ((branchbyte1 << 8) | branchbyte2);
        f.pc += offset - 3;
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
    } break;
    case getstatic:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;

      cp_info field_name;
      Class *class = load_class_and_field_name_from_field_ref(
          class_loader, method, index, &field_name);

      u32 value = get_static(
          class,
          (String) {
            .bytes = (char *)field_name.as.utf8_info.bytes, 
            .length = field_name.as.utf8_info.length
          });
    
      f_push(&f, value);

    } break;
    case putstatic:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;

      u32 value = f_pop(&f);

      cp_info field_name;

      Class *class = load_class_and_field_name_from_field_ref(
          class_loader, method, index, &field_name);

      set_static(
          class,
          (String) {
            .bytes = (char *)field_name.as.utf8_info.bytes,
            .length = field_name.as.utf8_info.length
          },
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
      u8 index = *(f.pc++);
      i8 const_value = *(f.pc++);

      f.locals[index] += const_value;
    } break;
    case invokestatic:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
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
          (String) {
            .bytes = (char *)class_name_constant.as.utf8_info.bytes,
            .length = class_name_constant.as.utf8_info.length
          });
      if (!class) {
        class = load_class(
            class_loader,
            (String) {
              .bytes = (char *)class_name_constant.as.utf8_info.bytes,
              .length = class_name_constant.as.utf8_info.length
            });
        // TODO: call <clinit>
      }

      Method *method = HashMap_get(
          class->method_map,
          (String) {
            .bytes = (char *)method_name_constant.as.utf8_info.bytes,
            .length = method_name_constant.as.utf8_info.length
          });

      code_attribute *method_code = method->code_attr;

      // Pop all args from the stack and store them in locals[] of the new frame
      method_descriptor method_descriptor = method->descriptor;

      Frame new_frame = create_frame_from_code_attribute(*method_code);

      // Copy all args from the callers stack to the callees local vars
      u64 num_args = sb_length(method_descriptor.parameter_types);
      memcpy(
          new_frame.locals,
          f.sp - num_args,
          num_args * sizeof(u64));
      // Pop all args
      f.sp = f.sp - num_args;

      sb_push(frames, f);
      f = new_frame;
    } break;
    case goto_instr:
    {
      u8 branchbyte1 = *(f.pc++);
      u8 branchbyte2 = *(f.pc++);

      int offset = (i16)((branchbyte1 << 8) | branchbyte2);
      f.pc += offset - 3;
    } break;
    case getfield:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;
 
      Object *this_ptr = (Object *)f_pop(&f);

      cp_info fieldref = method->constant_pool[index-1];
      cp_info name_and_type = method->constant_pool[
        fieldref.as.fieldref_info.name_and_type_index-1];
      cp_info fieldname = method->constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];

      u64 value = *((u64 *)HashMap_get(
          this_ptr->fields,
          (String) {
            .bytes = (char *)fieldname.as.utf8_info.bytes,
            .length = fieldname.as.utf8_info.length
          }));

      f_push(&f, value);
    } break;
    case putfield:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;
 
      u64 value = f_pop(&f);
      Object *this_ptr = (Object *)f_pop(&f);

      cp_info fieldref = method->constant_pool[index-1];
      cp_info name_and_type = method->constant_pool[
        fieldref.as.fieldref_info.name_and_type_index-1];
      cp_info fieldname = method->constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];

      u64 *heap_value = malloc(sizeof(u64));
      *heap_value = value;

      HashMap_insert(
          this_ptr->fields,
          (String) {
            .bytes = (char *)fieldname.as.utf8_info.bytes,
            .length = fieldname.as.utf8_info.length
          },
          heap_value);
    } break;
    case invokevirtual:
    {
      // https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-6.html#jvms-6.5.invokevirtual
      // TODO: copied from invokespecial => refactor
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;
 
      Object *this_ptr = (Object *)f_pop(&f);

      // Load the class
      cp_info methodref = method->constant_pool[index-1];
      cp_info class_info = method->constant_pool[methodref.as.methodref_info.class_index-1];
      cp_info class_name = method->constant_pool[class_info.as.class_info.name_index-1];
      Class *class = get_class(
          class_loader,
          (String) {
            .bytes = (char *)class_name.as.utf8_info.bytes, 
            .length = class_name.as.utf8_info.length
          });
      if (!class) {
        // TODO: Loas class
        break;
      }

      // Get the method
      cp_info name_and_type = method->constant_pool[
        methodref.as.methodref_info.name_and_type_index-1];
      cp_info name_info = method->constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];
      Method *method = HashMap_get(
          class->method_map, 
          (String) {
            .bytes = (char *)name_info.as.utf8_info.bytes,
            .length = name_info.as.utf8_info.length
          });
     
      assert(sb_length(method->descriptor.parameter_types) == 0); // Assert no args for now

      // Put this_ptr in locals[0]
      Frame new_frame = create_frame_from_code_attribute(*method->code_attr);
      new_frame.locals[0] = (u64) this_ptr;

      sb_push(frames, f);
      f = new_frame;
    } break;
    case invokespecial:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;

      Object *this_ptr = (Object *)f_pop(&f);

      // Load the class
      cp_info methodref = method->constant_pool[index-1];
      cp_info class_info = method->constant_pool[methodref.as.methodref_info.class_index-1];
      cp_info class_name = method->constant_pool[class_info.as.class_info.name_index-1];
      Class *class = get_class(
          class_loader,
          (String) {
            .bytes = (char *)class_name.as.utf8_info.bytes, 
            .length = class_name.as.utf8_info.length
          });
      if (!class) {
        // TODO: Loas class
        break;
      }

      // Get the method
      cp_info name_and_type = method->constant_pool[
        methodref.as.methodref_info.name_and_type_index-1];
      cp_info name_info = method->constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];
      Method *method = HashMap_get(
          class->method_map, 
          (String) {
            .bytes = (char *)name_info.as.utf8_info.bytes,
            .length = name_info.as.utf8_info.length
          });
     
      assert(sb_length(method->descriptor.parameter_types) == 0); // Assert no args for now

      // Put this_ptr in locals[0]
      Frame new_frame = create_frame_from_code_attribute(*method->code_attr);
      new_frame.locals[0] = (u64) this_ptr;

      sb_push(frames, f);
      f = new_frame;
    } break;
    case new:
    {
      u8 indexbyte1 = *(f.pc++);
      u8 indexbyte2 = *(f.pc++);
      u16 index = (indexbyte1 << 8) | indexbyte2;

      Class *class = load_class_from_constant_pool(class_loader, method->constant_pool, index);
      Object *obj = Object_create(class);

      // TODO: Should this take up two slots? Is max_stack_height still correct?
      f_push(&f, (u64) obj);

      // initialize instance vars to their default values
    } break;
    default: printf("Unhandled instruction with opcode %d\n", bytecode);
    }
  }

  printf("Returning without a \"return\" statement\n");

exit:
#ifdef DEBUG
  assert(f.sp == f.stack);
#endif
  free_frame(f);
}
