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

internal inline u8 read_immediate_u8(Frame *f) {
  return *(f->pc++);
}

#define ifCOND(operator)                          \
  do {                                            \
    int offset = read_immediate_i16(&f);          \
    u32 value = f_pop(&f);                        \
    if (value operator 0) { /* Branch succeeds */ \
      f.pc += offset - 3;                         \
    }                                             \
  } while(0)                                     \


#define iBINOP(operator)                            \
  do {                                              \
      f.sp -= 1; u32 value2 = *f.sp;                \
      u32 value1 = *(f.sp-1);                       \
      *(f.sp-1) = (int)value1 operator (int)value2; \
  } while(0)

#define ificmpCOND(operator)                              \
  do {                                                    \
      int offset = read_immediate_i16(&f);                \
      u32 value2 = f_pop(&f);                             \
      u32 value1 = f_pop(&f);                             \
      if (value1 operator value2) { /* Branch succeeds */ \
        f.pc += offset - 3;                               \
      }                                                   \
  } while(0)

enum ArrayType {
  T_BOOLEAN = 4,
  T_CHAR    = 5,
  T_FLOAT   = 6,
  T_DOUBLE  = 7,
  T_BYTE    = 8,
  T_SHORT   = 9,
  T_INT     = 10,
  T_LONG    = 11,
};

u8 ArrayType_byte_size(ArrayType type) {
  switch (type) {
  case T_BOOLEAN: return 1;
  case T_CHAR:    return 1;
  case T_FLOAT:   return 4;
  case T_DOUBLE:  return 8;
  case T_BYTE:    return 1;
  case T_SHORT:   return 2;
  case T_INT:     return 4;
  case T_LONG:    return 8;
  default:
  {
    // printf("Unknown array type %d\n", type);
    assert(false);
  }
  }
}

void execute(ClassLoader class_loader, Method *method) {
  static void* dispatch_table[] = {
      [0] = &&not_implemented,
      [1] = &&not_implemented,
      [2] = &&do_iconst_n, [3] = &&do_iconst_n, [4] = &&do_iconst_n, [5] = &&do_iconst_n,
      [6] = &&do_iconst_n, [7] = &&do_iconst_n, [8] = &&do_iconst_n,
      [9] = &&not_implemented,
      [10] = &&not_implemented,
      [11] = &&not_implemented,
      [12] = &&not_implemented,
      [13] = &&not_implemented,
      [14] = &&not_implemented,
      [15] = &&not_implemented,
      [16] = &&do_bipush,
      [17] = &&do_sipush,
      [18] = &&do_ldc,
      [19] = &&not_implemented,
      [20] = &&not_implemented,
      [21] = &&do_iload,
      [22] = &&not_implemented,
      [23] = &&not_implemented,
      [24] = &&not_implemented,
      [25] = &&do_aload,
      [26] = &&do_iload_n,
      [27] = &&do_iload_n,
      [28] = &&do_iload_n,
      [29] = &&do_iload_n,
      [30] = &&not_implemented,
      [31] = &&not_implemented,
      [32] = &&not_implemented,
      [33] = &&not_implemented,
      [34] = &&not_implemented,
      [35] = &&not_implemented,
      [36] = &&not_implemented,
      [37] = &&not_implemented,
      [38] = &&not_implemented,
      [39] = &&not_implemented,
      [40] = &&not_implemented,
      [41] = &&not_implemented,
      [42] = &&do_aload_n,
      [43] = &&do_aload_n,
      [44] = &&do_aload_n,
      [45] = &&do_aload_n,
      [46] = &&do_iaload,
      [47] = &&not_implemented,
      [48] = &&not_implemented,
      [49] = &&not_implemented,
      [50] = &&not_implemented,
      [51] = &&not_implemented,
      [52] = &&not_implemented,
      [53] = &&not_implemented,
      [54] = &&do_istore,
      [55] = &&not_implemented,
      [56] = &&not_implemented,
      [57] = &&not_implemented,
      [58] = &&not_implemented,
      [59] = &&do_istore_n,
      [60] = &&do_istore_n,
      [61] = &&do_istore_n,
      [62] = &&do_istore_n,
      [63] = &&not_implemented,
      [64] = &&not_implemented,
      [65] = &&not_implemented,
      [66] = &&not_implemented,
      [67] = &&not_implemented,
      [68] = &&not_implemented,
      [69] = &&not_implemented,
      [70] = &&not_implemented,
      [71] = &&not_implemented,
      [72] = &&not_implemented,
      [73] = &&not_implemented,
      [74] = &&not_implemented,
      [75] = &&do_astore_n,
      [76] = &&do_astore_n,
      [77] = &&do_astore_n,
      [78] = &&do_astore_n,
      [79] = &&do_iastore,
      [80] = &&not_implemented,
      [81] = &&not_implemented,
      [82] = &&not_implemented,
      [83] = &&not_implemented,
      [84] = &&not_implemented,
      [85] = &&not_implemented,
      [86] = &&not_implemented,
      [87] = &&do_pop,
      [88] = &&not_implemented,
      [89] = &&do_dup,
      [90] = &&not_implemented,
      [91] = &&not_implemented,
      [92] = &&not_implemented,
      [93] = &&not_implemented,
      [94] = &&not_implemented,
      [95] = &&not_implemented,
      [96] = &&do_iadd,
      [97] = &&not_implemented,
      [98] = &&not_implemented,
      [99] = &&not_implemented,
      [100] = &&do_isub,
      [101] = &&not_implemented,
      [102] = &&not_implemented,
      [103] = &&not_implemented,
      [104] = &&do_imul,
      [105] = &&not_implemented,
      [106] = &&not_implemented,
      [107] = &&not_implemented,
      [108] = &&not_implemented,
      [109] = &&not_implemented,
      [110] = &&not_implemented,
      [111] = &&not_implemented,
      [112] = &&not_implemented,
      [113] = &&not_implemented,
      [114] = &&not_implemented,
      [115] = &&not_implemented,
      [116] = &&not_implemented,
      [117] = &&not_implemented,
      [118] = &&not_implemented,
      [119] = &&not_implemented,
      [120] = &&not_implemented,
      [121] = &&not_implemented,
      [122] = &&not_implemented,
      [123] = &&not_implemented,
      [124] = &&not_implemented,
      [125] = &&not_implemented,
      [126] = &&not_implemented,
      [127] = &&not_implemented,
      [128] = &&not_implemented,
      [129] = &&not_implemented,
      [130] = &&not_implemented,
      [131] = &&not_implemented,
      [132] = &&do_iinc,
      [133] = &&not_implemented,
      [134] = &&not_implemented,
      [135] = &&not_implemented,
      [136] = &&not_implemented,
      [137] = &&not_implemented,
      [138] = &&not_implemented,
      [139] = &&not_implemented,
      [140] = &&not_implemented,
      [141] = &&not_implemented,
      [142] = &&not_implemented,
      [143] = &&not_implemented,
      [144] = &&not_implemented,
      [145] = &&not_implemented,
      [146] = &&not_implemented,
      [147] = &&not_implemented,
      [148] = &&not_implemented,
      [149] = &&not_implemented,
      [150] = &&not_implemented,
      [151] = &&not_implemented,
      [152] = &&not_implemented,
      [153] = &&do_ifeq,
      [154] = &&do_ifne,
      [155] = &&not_implemented,
      [156] = &&not_implemented,
      [157] = &&not_implemented,
      [158] = &&not_implemented,
      [159] = &&not_implemented,
      [160] = &&do_if_icmpne,
      [161] = &&not_implemented,
      [162] = &&do_if_icmpge,
      [163] = &&do_if_icmpgt,
      [164] = &&not_implemented,
      [165] = &&not_implemented,
      [166] = &&not_implemented,
      [167] = &&do_goto_instr,
      [168] = &&not_implemented,
      [169] = &&not_implemented,
      [170] = &&not_implemented,
      [171] = &&not_implemented,
      [172] = &&do_ireturn,
      [173] = &&not_implemented,
      [174] = &&not_implemented,
      [175] = &&not_implemented,
      [176] = &&not_implemented,
      [177] = &&do_return_void,
      [178] = &&do_getstatic,
      [179] = &&do_putstatic,
      [180] = &&do_getfield,
      [181] = &&do_putfield,
      [182] = &&do_invokevirtual,
      [183] = &&do_invokespecial,
      [184] = &&do_invokestatic,
      [185] = &&not_implemented,
      [186] = &&not_implemented,
      [187] = &&do_new_instr,
      [188] = &&do_new_array,
      [189] = &&not_implemented,
      [190] = &&not_implemented,
      [191] = &&not_implemented,
      [192] = &&not_implemented,
      [193] = &&not_implemented,
      [194] = &&not_implemented,
      [195] = &&not_implemented,
      [196] = &&not_implemented,
      [197] = &&not_implemented,
      [198] = &&not_implemented,
      [199] = &&not_implemented,
      [200] = &&not_implemented,
      [201] = &&not_implemented,
      [202] = &&not_implemented,
      [203] = &&not_implemented,
      [204] = &&not_implemented,
      [205] = &&not_implemented,
      [206] = &&not_implemented,
      [207] = &&not_implemented,
      [208] = &&not_implemented,
      [209] = &&not_implemented,
      [210] = &&not_implemented,
      [211] = &&not_implemented,
      [212] = &&not_implemented,
      [213] = &&not_implemented,
      [214] = &&not_implemented,
      [215] = &&not_implemented,
      [216] = &&not_implemented,
      [217] = &&not_implemented,
      [218] = &&not_implemented,
      [219] = &&not_implemented,
      [220] = &&not_implemented,
      [221] = &&not_implemented,
      [222] = &&not_implemented,
      [223] = &&not_implemented,
      [224] = &&not_implemented,
      [225] = &&not_implemented,
      [226] = &&not_implemented,
      [227] = &&not_implemented,
      [228] = &&not_implemented,
      [229] = &&not_implemented,
      [230] = &&not_implemented,
      [231] = &&not_implemented,
      [232] = &&not_implemented,
      [233] = &&not_implemented,
      [234] = &&not_implemented,
      [235] = &&not_implemented,
      [236] = &&not_implemented,
      [237] = &&not_implemented,
      [238] = &&not_implemented,
      [239] = &&not_implemented,
      [240] = &&not_implemented,
      [241] = &&not_implemented,
      [242] = &&not_implemented,
      [243] = &&not_implemented,
      [244] = &&not_implemented,
      [245] = &&not_implemented,
      [246] = &&not_implemented,
      [247] = &&not_implemented,
      [248] = &&not_implemented,
      [249] = &&not_implemented,
      [250] = &&not_implemented,
      [251] = &&not_implemented,
      [252] = &&not_implemented,
      [253] = &&not_implemented,
      [254] = &&not_implemented,
      [255] = &&not_implemented,
  };

#define DISPATCH() goto *dispatch_table[*f.pc++]
#define CURRENT_INSTR *(f.pc -1)

  assert(method);
  Vector<Frame> frames;
  /* Current frame */
  Frame f = create_frame_from_method(method);

  while (true) { // TODO(noah): Prevent overflows here
    DISPATCH();

do_iconst_n:
    {
      u8 n = CURRENT_INSTR - 3;
      *f.sp = n;
      f.sp += 1;
    } DISPATCH();
do_bipush:
    {
      u8 n = *(f.pc++);
      *f.sp = n;
      f.sp += 1;
    } DISPATCH();
do_sipush:
    {
      u16 byte = read_immediate_i16(&f);
      f_push(&f, byte);
    } DISPATCH();
do_ldc:
    {
      u8 index = read_immediate_u8(&f);
      f_push(&f, f.constant_pool[index-1].as.integer_value);
    } DISPATCH();
    /* iload_<n> */
do_iload_n:
    {
      u8 local_var_index = CURRENT_INSTR - 26;
      *f.sp = f.locals[local_var_index];
      f.sp += 1;
    } DISPATCH();
do_aload_n:
    {
      u16 local_var_index = CURRENT_INSTR - 42;
      f_push(&f, f.locals[local_var_index]);
    } DISPATCH();
do_iaload:
    {
      // TODO
    } DISPATCH();
do_istore_n:
    {
      u8 local_var_index = CURRENT_INSTR - 59;
      f.sp -= 1;
      f.locals[local_var_index] = *f.sp;
    } DISPATCH();
    /* astore_<n> */
do_astore_n:
    {
      u16 local_var_index = CURRENT_INSTR - 75;
      u64 reference = f_pop(&f);
      f.locals[local_var_index] = reference;
    } DISPATCH();
do_iastore:
    {
      // TODO
    } DISPATCH();
do_pop:
    { f.sp -= 1; } DISPATCH();
do_dup:
    { f_push(&f, *(f.sp-1)); } DISPATCH();
do_ifeq:
    { ifCOND(==); } DISPATCH();
do_ifne:
    { ifCOND(!=); } DISPATCH();
do_if_icmpgt:
    { ificmpCOND(>); } DISPATCH();
do_if_icmpge:
    { ificmpCOND(>=); } DISPATCH();
do_if_icmpne:
    { ificmpCOND(!=); } DISPATCH();
do_ireturn:
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
    } DISPATCH();
do_return_void:
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
    } DISPATCH();
do_getstatic:
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

    } DISPATCH();
do_putstatic:
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
    } DISPATCH();
do_imul: { iBINOP(*); } DISPATCH();
do_iadd: { iBINOP(+); } DISPATCH();
do_isub: { iBINOP(-); } DISPATCH();
do_iinc:
    {
      u8 index = *(f.pc++);
      i8 const_value = *(f.pc++);

      f.locals[index] += const_value;
    } DISPATCH();
do_invokestatic:
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
    } DISPATCH();
do_goto_instr:
    {
      int offset = read_immediate_i16(&f);

      f.pc += offset - 3;
    } DISPATCH();
do_getfield:
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
    } DISPATCH();
do_putfield:
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
    } DISPATCH();
do_invokevirtual:
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
        DISPATCH(); // Skip Object.<init> for now
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
    } DISPATCH();
do_invokespecial:
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
        DISPATCH(); // TODO
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
    } DISPATCH();
do_new_instr:
    {
      u16 index = read_immediate_i16(&f);

      Class *cls = load_class_from_constant_pool(class_loader, f.constant_pool, index);
      Object *obj = Object_create(cls);

      f_push(&f, (u64) obj);

      // initialize instance vars to their default values
    } DISPATCH();
do_new_array:
    {
      i32 count = f_pop(&f);
      u8 atype = read_immediate_u8(&f);

      i64 array_size = count * ArrayType_byte_size((ArrayType)atype);
      void *array = calloc(array_size, 2);
      f_push(&f, (u64) array);

    } DISPATCH();
do_iload:
do_aload:
do_istore:
not_implemented:
    {
      printf("Unhandled instruction with opcode %d\n", CURRENT_INSTR);
      exit(-1);
    }
    }

  printf("Returning without a \"return\" statement\n");

exit:
#ifdef DEBUG
  assert(f.sp == f.stack);
#endif
  free_frame(f);
}
