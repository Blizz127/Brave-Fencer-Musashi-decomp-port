/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches a sound command by id range (system sound, music, or sound effect with argument) with slot updates paused, then runs the deferred slot update if one was requested. */
#include "psx_types.h"

extern void func_8002D320(void);
extern void func_8002D904(u16);
extern void func_8002DC68(s32, u16);
extern void func_8002E138(u16, u16, u16);
extern void func_80030F80(void);
extern void func_80031BE0(void);
extern s16 D_800A4EFC;
extern u8 D_800A4F17;
extern u8 D_800A4F18;

void func_8002D59C(u16 id, u16 arg, u16 extra) {
    u8 *paused;

    D_800A4F17 = 1;
    if (id < 0x80) {
        func_8002E138(id, arg, extra);
    } else if (id >= 0x100) {
        if (id < 0x400) {
            func_8002D904(id);
        } else {
            func_8002DC68(id | (extra << 16), arg);
        }
    }
    paused = &D_800A4F17;
    *paused = D_800A4F18;
    if (D_800A4F18 != 0) {
        if (D_800A4EFC != 0) {
            D_800A4EFC--;
        }
        func_80030F80();
        func_8002D320();
        func_80031BE0();
        D_800A4F18 = 0;
        *paused = 0;
    }
}
