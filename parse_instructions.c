/* Note(Noah):
 * This is not needed at the moment since we don't parse all instructions ahead of time
 * but rather interpret the on the fly.
 *
 * Offsets used by goto instructions are specified by byte and not instruction anyways.
 * This code might be used later for pretty printing or debugging.
 */
#include "jvm.h"
#include "buffer.h"

typedef struct  {
  instruction_type type;
  union {
    struct { u8 byte; } single_byte;
    struct {
      u8 arg1;
      u8 arg2;
    } two_bytes;
  } as;
} instruction;

/* returns a stretchy buffer of instructions. */
instruction *parse_instructions(u64 length, u8 *code) {
  instruction *instructions = NULL;

  u64 offset = 0;
  while (offset < length) {
    u8 bytecode = *(code+offset); offset += 1;
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
    /* iload_<n> */
    case 26: case 27: case 28: case 29:
    {
      instr.type = iload_n;
      instr.as.single_byte.byte = bytecode - 26;
    } break;
    /* aload_<n> */
    case 42: case 43: case 44: case 45:
    {
      instr.type = aload_n;
      instr.as.single_byte.byte = bytecode - 42;
    } break;
    /* istore_<n> */
    case 59: case 60: case 61: case 62:
    {
      instr.type = istore_n;
      instr.as.single_byte.byte = bytecode - 59;
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
    case aload:
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
    case invokespecial:
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
