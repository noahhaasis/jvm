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

void execute(code_attribute method_code) {
  u1 *pc = method_code.code;
  u4 *stack = malloc(method_code.max_stack * sizeof(u4));
  u4 *sp = stack;
  u1 *locals = malloc(method_code.max_locals);


  while (pc < (method_code.code + method_code.code_length)) {
    u4 offset = 0;
    u1 bytecode = *pc;

    switch(bytecode) {
    /* iconst_<n> */
    case 2: case 3: case 4: case 5:
    case 6: case 7: case 8:
    {
      u1 n = bytecode - 3;
      *sp++ = n;
    } break;
    /* iload_<n> */
    case 26: case 27: case 28: case 29:
    {
      u1 local_var_index = bytecode - 3;
      *sp = locals[local_var_index];
      sp += 1;
    } break;
    /* istore_<n> */
    case 59: case 60: case 61: case 62:
    {
      u1 local_var_index = bytecode - 59;
      sp -= 1;
      locals[local_var_index] = *sp;
    } break;
    case ifne:
    {
      u1 branchbyte1 = *(pc+offset); offset += 1;
      u1 branchbyte2 = *(pc+offset); offset += 1;

      sp -= 1;
      u4 value = *sp;

      if (value != 0) { // Branch succeeds
        offset = (branchbyte1 << 8) | branchbyte2;
      }
    } break;
    case if_icmpgt:
    {
      u1 branchbyte1 = *(pc+offset); offset += 1;
      u1 branchbyte2 = *(pc+offset); offset += 1;

      sp -= 1; u4 value2 = *sp;
      sp -= 1; u4 value1 = *sp;

      if (value1 > value2) { // Branch succeeds
        offset = (branchbyte1 << 8) | branchbyte2;
      }
    } break;
    case ireturn:
    {
      sp -= 1;
      u4 value = *sp;
      // TODO
      printf("Returning int %d\n", value);
      goto exit;
    } break;
    case return_void:
    {
      printf("Returning void\n");
      goto exit;
    } break;
    case imul:
    {
      sp -= 1; u4 value2 = *sp;
      u4 value1 = *(sp-1);
      *sp = (int)value1 * (int)value2;
    } break;
    case iinc:
    {
      u1 index = *(pc + offset); offset += 1;
      i1 const_value =  *(pc + offset); offset += 1;

      locals[index] += const_value;
    } break;
    case goto_instr:
    {
      u1 branchbyte1 = *(pc+offset); offset += 1;
      u1 branchbyte2 = *(pc+offset); offset += 1;

      offset = (branchbyte1 << 8) | branchbyte2;
    } break;
    default: printf("Unhandled instruction with opcode %d\n", bytecode);
    }

    pc += offset;
  }

exit:
#ifdef DEBUG
  assert(sp == stack);
#endif

  free(stack);
  free(locals);
}
