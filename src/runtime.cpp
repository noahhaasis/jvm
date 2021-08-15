#include "runtime.h"

#include "Vector.h"

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

  Class *main_class = get_or_load_class(
      class_loader,
      String_from_null_terminated(class_name)
    );

  if (!main_class) {
    fprintf(stderr, "Failed to load class \"%s\"\n", class_name);
    return;
  }

  Method *main_method = main_class->method_map->get(
      (String) {
        .length = strlen("main"),
        .bytes = (char *)"main",
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

internal Class *load_class_and_field_name_from_field_ref(
    ClassLoader class_loader, cp_info *constant_pool, u16 index,
    Utf8Info *out_field_name) {
      // Field resolution
    FieldrefInfo fieldref_info = constant_pool[index-1].as.fieldref_info;
    Utf8Info *class_name = fieldref_info.class_info->name;
      
    Class *cls = get_or_load_class(class_loader, (String) {
        .length = class_name->length,
        .bytes = (char *)class_name->bytes,
      });

    *out_field_name = *fieldref_info.name_and_type->name;

    return cls;
}

Class *load_class_from_constant_pool(ClassLoader class_loader, cp_info *constant_pool, u16 index) {
  cp_info class_info = constant_pool[index-1];
  assert(class_info.tag == CONSTANT_Class);
  Utf8Info *class_name = class_info.as.class_info.name;
  
  Class *cls = get_or_load_class(
      class_loader,
      (String) {
        .length = class_name->length,
        .bytes = (char *)class_name->bytes,
      });

  return cls;
}

FieldInfo *find_instance_field(Class *cls, String fieldname)
{
  Class *current_class = cls;
  // Walk up the inheritance hierarchy to find the field
  while (current_class) {
    FieldInfo *info = current_class->instance_field_map->get(fieldname);
    if (info) {
      return info;
    }
    current_class = current_class->super_class;
  }
  return NULL;
}

struct Object {
  Class *cls;
  u8 data[];
};

Object *Object_create(Class *cls) {
  Object *obj = (Object *)calloc(sizeof(Object) + cls->object_body_size, 1);
  obj->cls = cls;
  return obj;
}

internal inline i16 read_immediate_i16(Frame *f) {
  u8 branchbyte1 = *(f->pc++);
  u8 branchbyte2 = *(f->pc++);
  return (i16)((branchbyte1 << 8) | branchbyte2);
}

#define ifCOND(operator)                              \
      do {                                            \
        int offset = read_immediate_i16(&f);          \
        u32 value = f_pop(&f);                        \
        if (value operator 0) { /* Branch succeeds */ \
          f.pc += offset - 3;                         \
        }                                             \
      } while(0);                                     \


#define iBINOP(operator)                            \
      f.sp -= 1; u32 value2 = *f.sp;                \
      u32 value1 = *(f.sp-1);                       \
      *(f.sp-1) = (int)value1 operator (int)value2; \

void execute(ClassLoader class_loader, Method *method) {
  assert(method);
  Vector<Frame> frames;
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
    case sipush:
    {
      u16 byte = read_immediate_i16(&f);
      f_push(&f, byte);
    } break;
    case ldc:
    {
      u8 index = read_immediate_u8(&f);
      f_push(&f, f.constant_pool[index-1].as.integer_value);
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
    case ifeq: { ifCOND(==) } break;
    case ifne: { ifCOND(!=) } break;
    case if_icmpgt: { ificmpCOND(>); } break;
    case if_icmpge: { ificmpCOND(>=); } break;
    case if_icmpne: { ificmpCOND(!=); } break;
    case ireturn:
    {
      u32 value = f_pop(&f);
#ifdef DEBUG
      printf("Returning int %d\n", value);
#endif
      if (frames.empty()) {
        goto exit;
      }
#ifdef DEBUG
      assert(f.sp == f.stack);
#endif
      free_frame(f);
      f = frames.pop();

      // push the return value
      f_push(&f, value);
    } break;
    case return_void:
    {
#ifdef DEBUG
      printf("Returning void\n");
#endif
      if (frames.empty()) {
        goto exit;
      }
#ifdef DEBUG
      assert(f.sp == f.stack);
#endif
      free_frame(f);
      f = frames.pop();
    } break;
    case getstatic:
    {
      u16 index = read_immediate_i16(&f);

      Utf8Info field_name;
      Class *cls = load_class_and_field_name_from_field_ref(
          class_loader, f.constant_pool, index, &field_name);

      u32 value = get_static(
          cls,
          (String) {
            .length = field_name.length,
            .bytes = (char *)field_name.bytes,
          });
    
      f_push(&f, value);

    } break;
    case putstatic:
    {
      u16 index = read_immediate_i16(&f);

      u32 value = f_pop(&f);

      Utf8Info field_name;

      Class *cls = load_class_and_field_name_from_field_ref(
          class_loader, f.constant_pool, index, &field_name);

      set_static(
          cls,
          (String) {
            .length = field_name.length,
            .bytes = (char *)field_name.bytes,
          },
          value);
    } break;
    case imul: { iBINOP(*); } break;
    case iadd: { iBINOP(+); } break;
    case isub: { iBINOP(-); } break;
    case iinc:
    {
      u8 index = *(f.pc++);
      i8 const_value = *(f.pc++);

      f.locals[index] += const_value;
    } break;
    case invokestatic:
    {
      u16 index = read_immediate_i16(&f);

      Utf8Info *method_name = f.constant_pool[index-1].as.methodref_info.name_and_type->name;
      Utf8Info *class_name = f.constant_pool[index-1].as.methodref_info.class_info->name;

      Class *cls = get_or_load_class(
          class_loader,
          (String) {
            .length = class_name->length,
            .bytes = (char *)class_name->bytes,
          });

      Method *method = cls->method_map->get(
          (String) {
            .length = method_name->length,
            .bytes = (char *)method_name->bytes,
          });

      // Pop all args from the stack and store them in locals[] of the new frame
      method_descriptor method_descriptor = method->descriptor;

      Frame new_frame = create_frame_from_method(method);

      // Copy all args from the callers stack to the callees local vars
      u64 num_args = method_descriptor.parameter_types.length();
      memcpy(
          new_frame.locals,
          f.sp - num_args,
          num_args * sizeof(u64));
      // Pop all args
      f.sp = f.sp - num_args;

      frames.push(f);
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

      Utf8Info *fieldname = f.constant_pool[index-1].as.fieldref_info.name_and_type->name;

      FieldInfo *field_info = find_instance_field(this_ptr->cls,
          (String) {
            .length = fieldname->length,
            .bytes = (char *)fieldname->bytes,
          });

      u64 value = ((u64 *)this_ptr->data)[field_info->index];

      f_push(&f, value);
    } break;
    case putfield:
    {
      u16 index = read_immediate_i16(&f);
 
      u64 value = f_pop(&f);
      Object *this_ptr = (Object *)f_pop(&f);

      Utf8Info *fieldname = f.constant_pool[index-1].as.fieldref_info.name_and_type->name;

      FieldInfo *field_info = find_instance_field(this_ptr->cls,
          (String) {
            .length = fieldname->length,
            .bytes = (char *)fieldname->bytes,
          });
      ((u64 *)this_ptr->data)[field_info->index] = value;
    } break;
    case invokevirtual:
    {
      // https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-6.html#jvms-6.5.invokevirtual
      // TODO: copied from invokespecial => refactor
      u16 index = read_immediate_i16(&f);

      // Load the class
      MethodrefInfo methodref = f.constant_pool[index-1].as.methodref_info;
      Utf8Info *class_name = methodref.class_info->name;
      Class *cls = get_or_load_class(
          class_loader,
          (String) {
            .length = class_name->length,
            .bytes = (char *)class_name->bytes,
          });

      if (!cls) {
        f.sp -= 1; // Pop this_ptr
        break; // Skip Object.<init> for now
      }

      // Get the method
      Utf8Info *method_name = methodref.name_and_type->name;
      Method *method = cls->method_map->get(
          (String) {
            .length = method_name->length,
            .bytes = (char *)method_name->bytes,
          });
     
      Frame new_frame = create_frame_from_method(method);

      // Copy all args + this_ptr from the callers stack to the callees local vars
      u64 num_args = method->descriptor.parameter_types.length() + 1;
      memcpy(
          new_frame.locals,
          f.sp - num_args,
          num_args * sizeof(u64));

      // Pop all args
      f.sp = f.sp - num_args;

      frames.push(f);
      f = new_frame;
    } break;
    case invokespecial:
    {
      u16 index = read_immediate_i16(&f);

      // Load the class
      MethodrefInfo methodref = f.constant_pool[index-1].as.methodref_info;
      Utf8Info *class_name = methodref.class_info->name;
      Class *cls = get_or_load_class(
          class_loader,
          (String) {
            .length = class_name->length,
            .bytes = (char *)class_name->bytes,
          });
      if (!cls) {
        f.sp -= 1; // Pop this_ptr
        break; // TODO
      }

      // Get the method
      Utf8Info *method_name = methodref.name_and_type->name;
      Method *method = cls->method_map->get( 
          (String) {
            .length = method_name->length,
            .bytes = (char *)method_name->bytes,
          });
     
      // Put this_ptr in locals[0]
      Frame new_frame = create_frame_from_method(method);

      // Copy all args + this_ptr from the callers stack to the callees local vars
      u64 num_args = method->descriptor.parameter_types.length() + 1;
      memcpy(
          new_frame.locals,
          f.sp - num_args,
          num_args * sizeof(u64));

      // Pop all args
      f.sp = f.sp - num_args;

      frames.push(f);
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
    default:
    {
      printf("Unhandled instruction with opcode %d\n", bytecode);
      exit(-1);
    }
    }
  }

  printf("Returning without a \"return\" statement\n");

exit:
#ifdef DEBUG
  assert(f.sp == f.stack);
#endif
  free_frame(f);
}
