/* Owner re-derivation from retail assembly (clean input set). */
/* Applies the current fade level to func_8003EDE8, steps it down, and when it reaches 0 stops the fade and calls func_8002EE90. */
#include "psx_types.h"

extern void func_8002EE90(void);
extern void func_8003EDE8(s32, s32, s32);
extern s32 D_800A46B4[];
extern s16 D_800A46B8;
extern s8 D_800A46BA;
extern s16 D_800A4EF4;

void func_8002D29C(void) {
    s32 level;
    s32 scaled;

    level = D_800A46B4[0];
    scaled = (level * D_800A4EF4) >> 16;
    func_8003EDE8(0, scaled, scaled);
    level -= D_800A46B8;
    if (level <= 0) {
        D_800A46BA = 0;
        level = 0;
        func_8002EE90();
    }
    D_800A46B4[0] = level;
}
