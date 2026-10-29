/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern s32 D_801B6438;
extern void func_80147078(void *o, u16 a);
extern void func_801578C0(void *o);

void func_80157788(void *o) {
    D_801B6438 = 0x12C;
    func_80147078(o, 7);
    func_801578C0(o);
}
