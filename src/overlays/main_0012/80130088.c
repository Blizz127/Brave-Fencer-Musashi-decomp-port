/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 func_8012BCCC(void *o);
extern s32 func_80131AC8(void *o, s32 a);

void func_80130088(void *o) {
    if (func_8012BCCC(o) <= 0x8FFF) {
        if (func_80131AC8(o, 0x32) == 0) {
            func_80131AC8(o, 0x33);
        }
    } else {
        if (func_80131AC8(o, 0x33) == 0) {
            func_80131AC8(o, 0x32);
        }
    }
}
