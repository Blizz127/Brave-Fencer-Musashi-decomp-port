/* Independently re-derived from retail assembly; clean input set. */
#include "game_option_values.h"

extern s32 func_80028D9C(void);
extern void func_8002D4C8(u16 command, u16 argument);

void func_800116E0(void) {
    func_8002D4C8(0x21, func_80028FBC() & 0xFFFF);
    func_8002D4C8(0x22, func_80029000() & 0xFFFF);
    if (func_80028D9C() == 0) {
        func_8002D4C8(0xC, 0);
        return;
    }
    func_8002D4C8(0xB, 0);
}
