/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern u8 D_80078EC0;
extern void func_80146A6C(s32 a0, void *a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6);
extern void func_8015E9B8(void *o);

void func_8015E95C(void *o) {
    if ((D_80078EC0 & 0x80) == 0) {
        func_80146A6C(0x14, o, 0, 0, 0, 0, 0);
    }
    func_8015E9B8(o);
}
