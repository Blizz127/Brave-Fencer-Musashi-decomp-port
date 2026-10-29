/* Owner re-derivation from retail assembly (clean input set). */
/* If bit 2 of D_800A4E8E is set and the handle D_800A4E86 is valid, either restarts via func_8002EC10 or stops it and sets bit 4 of the flags. */
#include "psx_types.h"

extern u16 D_800A4E8E;
extern s16 D_800A4E86;
extern s32 func_8003836C(s32 h);
extern s32 func_800381E4(s32 h, s32 a);
extern void func_800383A4(s32 h);
extern void func_8002EC10(void);

void func_8002EA10(void) {
    u16 *flags = &D_800A4E8E;

    if (*flags & 4) {
        if ((s16)func_8003836C(D_800A4E86) != 0) {
            if (func_800381E4(D_800A4E86, 0) != 0) {
                func_8002EC10();
            } else {
                func_800383A4(D_800A4E86);
                *flags |= 0x10;
            }
        }
    }
}
