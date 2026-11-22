/* Append one script request and its command-specific operands to the word ring. */
#include "mine_script_queue.h"

static __inline__ const u16 *append_word(const u16 *command)
{
    u16 value = *command++;
    s16 index = D_801A0BDC;
    D_801A07D8[index] = value;
    D_801A0BDC = (index + 1) & 0x1ff;
    return command;
}

void func_80179B74(const u16 *command)
{
#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
    if (!command || (u16)D_801A0BDC >= 512) return;
#endif
    s16 opcode = *command;
    command = append_word(command);
    switch (opcode) {
    case 0: case 7: case 8: case 9: case 10: case 11: case 12:
    case 13: case 14: case 15: case 16: case 17: case 21: case 22:
    case 24: case 25: case 26: case 27: case 30: case 33: case 34:
        break;
    case 1: case 5:
        command = append_word(command);
        command = append_word(command);
        command = append_word(command);
        break;
    case 2: case 3: case 4: case 6: case 18: case 20:
        command = append_word(command);
        break;
    case 19:
        command = append_word(command);
        command = append_word(command);
        break;
    case 23:
        command = append_word(command);
        command = append_word(command);
        command = append_word(command);
        command = append_word(command);
        break;
    case 31:
        command = append_word(command);
        break;
    case 28: case 29: case 32:
        command = append_word(command);
        command = append_word(command);
        command = append_word(command);
        command = append_word(command);
        break;
    }
}
