#include "jvm.h"
#include "buffer.h"

void execute(code_attribute method_code) {
  u1 *pc = method_code.code;
  u4 *stack = malloc(method_code.max_stack * sizeof(u4));
  u4 *sp = stack;
  u4 *locals = malloc(method_code.max_locals);


  while (pc < (method_code.code + method_code.code_length)) {
    i4 offset = 1;
    u1 bytecode = *pc;

    switch(bytecode) {
    /* iconst_<n> */
    case 2: case 3: case 4: case 5:
    case 6: case 7: case 8:
    {
      u1 n = bytecode - 3;
      *sp = n;
      sp += 1;
    } break;
    /* iload_<n> */
    case 26: case 27: case 28: case 29:
    {
      u1 local_var_index = bytecode - 26;
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
        offset = (i2)((branchbyte1 << 8) | branchbyte2);
      }
    } break;
    case if_icmpgt:
    {
      u1 branchbyte1 = *(pc+offset); offset += 1;
      u1 branchbyte2 = *(pc+offset); offset += 1;

      sp -= 1; u4 value2 = *sp;
      sp -= 1; u4 value1 = *sp;

      if (value1 > value2) { // Branch succeeds
        offset = (i2) ((branchbyte1 << 8) | branchbyte2);
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
      *(sp-1) = (int)value1 * (int)value2;
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

      offset = (i2)((branchbyte1 << 8) | branchbyte2);
    } break;
    default: printf("Unhandled instruction with opcode %d\n", bytecode);
    }

    pc += offset;
  }

  printf("Returning without a \"return\" statement\n");

exit:
#ifdef DEBUG
  assert(sp == stack);
#endif

  free(stack);
  free(locals);
}
