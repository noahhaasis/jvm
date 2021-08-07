#include "runtime.h"

#include "buffer.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/* TODO(noah):
 * Instead of having a seperate frame stack just push
 * the pointers on the stack and pop them.
 */

struct Frame {
  u8 *pc;
  u64 *stack;
  u64 *sp;
  u64 *locals;
  cp_info *constant_pool;
};

Frame create_frame_from_method(Method *method) {
  code_attribute code_attr = *method->code_attr;
  u64 *stack =
    code_attr.max_stack > 0
    ? (u64 *)malloc(code_attr.max_stack * sizeof(u64))
    : NULL;
  return (Frame) {
    .pc = code_attr.code,
    .stack = stack,
    .sp = stack,
    .locals =
      code_attr.max_locals > 0
      ? (u64 *)malloc(code_attr.max_locals * sizeof(u64))
      : NULL,
    .constant_pool = method->constant_pool,
  };
}

void free_frame(Frame frame) {
  if (frame.stack)  free(frame.stack);
  if (frame.locals) free(frame.locals);
}

void execute_main(char *class_name) {
  char cwd[256];
  getcwd(cwd, 256);
  char *paths[2];
  paths[0] = (char *) "/home/haschisch/Projects/cl/test";
  paths[1] = cwd;
  ClassLoader class_loader = ClassLoader_create(paths, 2);

  Class *main_class = load_class(
      class_loader,
      String_from_null_terminated(class_name)
    );

  if (!main_class) {
    fprintf(stderr, "Failed to load class \"%s\"\n", class_name);
    return;
  }

  // call <clinit>
  Method *clinit = main_class->method_map->get(
      (String) {
        .length = strlen("<clinit>"),
        .bytes = "<clinit>",
      });
  execute(class_loader, clinit);


  Method *main_method = main_class->method_map->get(
      (String) {
        .length = strlen("main"),
        .bytes = "main",
      }); // TODO
  execute(class_loader, main_method);
  // ClassLoader_destroy(&);
}

internal inline u64 f_pop(Frame *f) {
  f->sp -= 1;
  return *f->sp;
}

internal inline void f_push(Frame *f, u64 value) {
  *f->sp = value;
  f->sp += 1;
}

internal Class *get_or_load_class(ClassLoader class_loader, String classname) {
  Class * cls = get_class(class_loader, classname);
  if (cls) {
    return cls;
  }

  cls = load_class(class_loader, classname);
  if (!cls) {
    fprintf(stderr, "Failed to find class \"%.*s\"\n", classname.length, classname.bytes);
    return NULL;
  }

  Method *clinit = cls->method_map->get(
    (String) {
      .length = strlen("<clinit>"),
      .bytes = "<clinit>",
    });
  if (clinit) {
    execute(class_loader, clinit);
  }

  return cls;
}

internal Class *load_class_and_field_name_from_field_ref(
    ClassLoader class_loader, cp_info *constant_pool, u16 index,
    cp_info *out_field_name) {
      // Field resolution
    cp_info fieldref_info = constant_pool[index-1];
    cp_info class_info = constant_pool[
      fieldref_info.as.fieldref_info.class_index-1];
    cp_info class_name = constant_pool[
      class_info.as.class_info.name_index-1];
      
    Class *cls = get_or_load_class(class_loader, (String) {
        .length = class_name.as.utf8_info.length,
        .bytes = (char *)class_name.as.utf8_info.bytes,
      });

    cp_info name_and_type = constant_pool[
      fieldref_info.as.fieldref_info.name_and_type_index-1];
    *out_field_name = constant_pool[
      name_and_type.as.name_and_type_info.name_index-1];

    return cls;
}

Class *load_class_from_constant_pool(ClassLoader class_loader, cp_info *constant_pool, u16 index) {
  cp_info class_info = constant_pool[index-1];
  assert(class_info.tag == CONSTANT_Class);
  cp_info class_name = constant_pool[class_info.as.class_info.name_index - 1];
  
  Class *cls = get_or_load_class(
      class_loader,
      (String) {
        .length = class_name.as.utf8_info.length,
        .bytes = (char *)class_name.as.utf8_info.bytes,
      });

  return cls;
}

// Well this is just obviously bad :^)
struct Object {
  Class *cls;
  // HashMap<string field, u32 value>
  HashMap<u64> *fields;
};

Object *Object_create(Class *cls) {
  Object *obj = (Object *)malloc(sizeof(Object));
  obj->cls = cls;
  obj->fields = new HashMap<u64>();
  return obj;
}

internal inline i16 read_immediate_i16(Frame *f) {
  u8 branchbyte1 = *(f->pc++);
  u8 branchbyte2 = *(f->pc++);
  return (i16)((branchbyte1 << 8) | branchbyte2);
}

void execute(ClassLoader class_loader, Method *method) {
  assert(method);

  // TODO(noah): Store the current class?
  Frame *frames = NULL;
  /* Current frame */
  Frame f = create_frame_from_method(method);

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
      int offset = read_immediate_i16(&f);

      f.sp -= 1;
      u32 value = *f.sp;

      if (value == 0) { // Branch succeeds
        f.pc += offset - 3;
      }
    } break;
    case ifne:
    {
      int offset = read_immediate_i16(&f);
      u32 value = f_pop(&f);

      if (value != 0) { // Branch succeeds
        f.pc += offset - 3;
      }
    } break;
    case if_icmpgt:
    {
      int offset = read_immediate_i16(&f);

      u32 value2 = f_pop(&f);
      u32 value1 = f_pop(&f);

      if (value1 > value2) { // Branch succeeds
        f.pc += offset - 3;
      }
    } break;
    case if_icmpne:
    {
      int offset = read_immediate_i16(&f);

      u32 value2 = f_pop(&f);
      u32 value1 = f_pop(&f);

      if (value1 != value2) { // Branch succeeds
        f.pc += offset - 3;
      }
    } break;
    case ireturn:
    {
      u32 value = f_pop(&f);
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
      f_push(&f, value);
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
      u16 index = read_immediate_i16(&f);

      cp_info field_name;
      Class *cls = load_class_and_field_name_from_field_ref(
          class_loader, f.constant_pool, index, &field_name);

      u32 value = get_static(
          cls,
          (String) {
            .length = field_name.as.utf8_info.length,
            .bytes = (char *)field_name.as.utf8_info.bytes, 
          });
    
      f_push(&f, value);

    } break;
    case putstatic:
    {
      u16 index = read_immediate_i16(&f);

      u32 value = f_pop(&f);

      cp_info field_name;

      Class *cls = load_class_and_field_name_from_field_ref(
          class_loader, f.constant_pool, index, &field_name);

      set_static(
          cls,
          (String) {
            .length = field_name.as.utf8_info.length,
            .bytes = (char *)field_name.as.utf8_info.bytes,
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
      u16 index = read_immediate_i16(&f);

      u16 name_and_type_index =
        f.constant_pool[index-1].as.methodref_info.name_and_type_index;
      u16 name_index =
        f.constant_pool[name_and_type_index-1].as.name_and_type_info.name_index;
      cp_info method_name_constant = f.constant_pool[name_index-1];

      u16 class_index =
        f.constant_pool[index-1].as.methodref_info.class_index;
      u16 class_name_index =
        f.constant_pool[class_index-1].as.class_info.name_index;
      cp_info class_name_constant = f.constant_pool[class_name_index-1];

      Class *cls = get_or_load_class(
          class_loader,
          (String) {
            .length = class_name_constant.as.utf8_info.length,
            .bytes = (char *)class_name_constant.as.utf8_info.bytes,
          });

      Method *method = cls->method_map->get(
          (String) {
            .length = method_name_constant.as.utf8_info.length,
            .bytes = (char *)method_name_constant.as.utf8_info.bytes,
          });

      // Pop all args from the stack and store them in locals[] of the new frame
      method_descriptor method_descriptor = method->descriptor;

      Frame new_frame = create_frame_from_method(method);

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
      int offset = read_immediate_i16(&f);

      f.pc += offset - 3;
    } break;
    case getfield:
    {
      u16 index = read_immediate_i16(&f);
 
      Object *this_ptr = (Object *)f_pop(&f);

      cp_info fieldref = f.constant_pool[index-1];
      cp_info name_and_type = f.constant_pool[
        fieldref.as.fieldref_info.name_and_type_index-1];
      cp_info fieldname = f.constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];

      u64 value = *this_ptr->fields->get(
          (String) {
            .length = fieldname.as.utf8_info.length,
            .bytes = (char *)fieldname.as.utf8_info.bytes,
          });

      f_push(&f, value);
    } break;
    case putfield:
    {
      u16 index = read_immediate_i16(&f);
 
      u64 value = f_pop(&f);
      Object *this_ptr = (Object *)f_pop(&f);

      cp_info fieldref = f.constant_pool[index-1];
      cp_info name_and_type = f.constant_pool[
        fieldref.as.fieldref_info.name_and_type_index-1];
      cp_info fieldname = f.constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];

      u64 *heap_value = (u64 *)malloc(sizeof(u64));
      *heap_value = value;

      this_ptr->fields->insert(
          (String) {
            .length = fieldname.as.utf8_info.length,
            .bytes = (char *)fieldname.as.utf8_info.bytes,
          }, heap_value);
    } break;
    case invokevirtual:
    {
      // https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-6.html#jvms-6.5.invokevirtual
      // TODO: copied from invokespecial => refactor
      u16 index = read_immediate_i16(&f);

      // Load the class
      cp_info methodref = f.constant_pool[index-1];
      cp_info class_info = f.constant_pool[methodref.as.methodref_info.class_index-1];
      cp_info class_name = f.constant_pool[class_info.as.class_info.name_index-1];
      Class *cls = get_or_load_class(
          class_loader,
          (String) {
            .length = class_name.as.utf8_info.length,
            .bytes = (char *)class_name.as.utf8_info.bytes, 
          });

      if (!cls) {
        f.sp -= 1; // Pop this_ptr
        break; // Skip Object.<init> for now
      }

      // Get the method
      cp_info name_and_type = f.constant_pool[
        methodref.as.methodref_info.name_and_type_index-1];
      cp_info name_info = f.constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];
      Method *method = cls->method_map->get(
          (String) {
            .length = name_info.as.utf8_info.length,
            .bytes = (char *)name_info.as.utf8_info.bytes,
          });
     
      Frame new_frame = create_frame_from_method(method);

      // Copy all args + this_ptr from the callers stack to the callees local vars
      u64 num_args = sb_length(method->descriptor.parameter_types) + 1;
      memcpy(
          new_frame.locals,
          f.sp - num_args,
          num_args * sizeof(u64));

      // Pop all args
      f.sp = f.sp - num_args;

      sb_push(frames, f);
      f = new_frame;
    } break;
    case invokespecial:
    {
      u16 index = read_immediate_i16(&f);

      // Load the class
      cp_info methodref = f.constant_pool[index-1];
      cp_info class_info = f.constant_pool[methodref.as.methodref_info.class_index-1];
      cp_info class_name = f.constant_pool[class_info.as.class_info.name_index-1];
      Class *cls = get_or_load_class(
          class_loader,
          (String) {
            .length = class_name.as.utf8_info.length,
            .bytes = (char *)class_name.as.utf8_info.bytes, 
          });
      if (!cls) {
        f.sp -= 1; // Pop this_ptr
        break; // TODO
      }

      // Get the method
      cp_info name_and_type = f.constant_pool[
        methodref.as.methodref_info.name_and_type_index-1];
      cp_info name_info = f.constant_pool[
        name_and_type.as.name_and_type_info.name_index-1];
      Method *method = cls->method_map->get( 
          (String) {
            .length = name_info.as.utf8_info.length,
            .bytes = (char *)name_info.as.utf8_info.bytes,
          });
     
      // Put this_ptr in locals[0]
      Frame new_frame = create_frame_from_method(method);

      // Copy all args + this_ptr from the callers stack to the callees local vars
      u64 num_args = sb_length(method->descriptor.parameter_types) + 1;
      memcpy(
          new_frame.locals,
          f.sp - num_args,
          num_args * sizeof(u64));

      // Pop all args
      f.sp = f.sp - num_args;

      sb_push(frames, f);
      f = new_frame;
    } break;
    case new_instr:
    {
      u16 index = read_immediate_i16(&f);

      Class *cls = load_class_from_constant_pool(class_loader, f.constant_pool, index);
      Object *obj = Object_create(cls);

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
