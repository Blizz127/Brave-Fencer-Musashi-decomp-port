/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 func_8012BCCC(void *o);
extern s32 func_80131AC8(void *o, s32 a);

void func_8013001C(void *o) {
    if (func_8012BCCC(o) <= 0x8FFF) {
        if (func_80131AC8(o, 0x30) == 0) {
            func_80131AC8(o, 0x31);
        }
    } else {
        if (func_80131AC8(o, 0x31) == 0) {
            func_80131AC8(o, 0x30);
        }
    }
}
