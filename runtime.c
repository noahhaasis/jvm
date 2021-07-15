#include "jvm.h"
#include "buffer.h"

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
 *
 * There are 149 instructions ( probably :^) )
 */

typedef enum {
  iconst        = 2, /* 2 - 8 ; Push iconst_<n>*/
  ldc           = 18,
  iload         = 21,
  iadd          = 96,
  istore        = 54,
  imul          = 104,
  iinc          = 132,
  ifne          = 154,
  if_icmpgt     = 163,
  goto_instr    = 167,
  ireturn       = 172,
  return_void   = 177,
  getstatic     = 178,
  invokevirtual = 182,
  invokestatic  = 184,
  undefined,
} instruction_type;

typedef struct  {
  instruction_type type;
  union {
    struct { u1 byte; } single_byte;
    struct {
      u1 arg1;
      u1 arg2;
    } two_bytes;
  } as;
} instruction;

/* returns a stretchy buffer of instructions. */
instruction *parse_instructions(u8 length, u1 *code) {
  instruction *instructions = NULL;

  u8 offset = 0;
  while (offset < length) {
    u1 bytecode = *(code+offset); offset += 1;
    instruction instr = {};

    instr.type = bytecode;

    switch (bytecode) {
    /* iconst_<n> */
    case 2: case 3: case 4: case 5:
    case 6: case 7: case 8:
    {
      instr.type = iconst;
      instr.as.single_byte.byte = bytecode - 3;
    } break;
    /* Instructions without args */
    case iadd:
    case imul:
    case ireturn:
    case return_void:
    {} break;
    /* Instructions with a single byte arg */
    case ldc:
    case iload:
    case istore:
    {
      instr.as.single_byte.byte = *(code+offset);
      offset += 1;
    } break;
    /* Instructions with two byte sized args */
    case iinc:
    case ifne:
    case if_icmpgt:
    case goto_instr:
    case getstatic:
    case invokevirtual:
    case invokestatic:
    {
      instr.as.two_bytes.arg1 = *(code+offset); offset += 1;
      instr.as.two_bytes.arg2 = *(code+offset); offset += 1;
    } break;
    /* Unknown instruction */
    default:
    {
      instr.type = undefined;
      printf("Unknown instruction %d\n", bytecode);
    }
    }

    sb_push(instructions, instr);
  }

#ifdef DEBUG
  assert(offset == length);
#endif

  return instructions;
}

void execute(code_attribute method_code) {
  /*
  u1* pc = method_code.code;
  while (pc < method_code.code + method_code.code_length) {
    int num_bytes_consumed = 0;
    instruction ins = fetch_instruction(pc, &num_bytes_consumed);
    pc += num_bytes_consumed;

    // TODO: Execute the instruction
    switch(ins.type) {
    case ldc:
    {} break;
    case iadd:
    {} break;
    case return_void:
    {} break;
    case iconst:
    {} break;
    default: 
    }
  } 
  */
}
