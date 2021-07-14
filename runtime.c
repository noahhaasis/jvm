#include "jvm.h"

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

instruction fetch_instruction(u1* bytecode, int *out_num_bytes_consumed) {
  instruction ins = (instruction) {};
  // TODO
  return ins;
}

void execute(code_attribute method_code) {
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

}
