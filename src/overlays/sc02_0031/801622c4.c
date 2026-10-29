/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80029124(s32 a0, s32 a1);
extern void func_800291C8(s32 a0, s32 a1);
extern void func_80016714(void *p, s32 n);
extern void *D_801B6458[];

void func_801622C4(void) {
    s32 i;
    void **p;

    func_80029124(0x141, 1);
    func_800291C8(8, 0);
    i = 0;
    p = D_801B6458;
    for (; i < 3; i++) {
        func_80016714(*p, 0x38);
        p = p + 1;
    }
}
