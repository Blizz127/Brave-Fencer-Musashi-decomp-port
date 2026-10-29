#include "psx_types.h"

extern void func_80139A8C(u8 *o);

void func_80139DF4(u8 *o) {
    if (o[0xC] == 0 || --o[0xC] == 0) {
        *(u16 *)(o + 4) = 2;
        return;
    }
    func_80139A8C(o);
    if (*(u32 *)(o + 8) & 0x20) {
        u32 f;
        *(u16 *)(o + 4) = 2;
        f = *(u32 *)(o + 8);
        o[0xC] = 0;
        if ((f & 0x20000) == 0) {
            *(u32 *)(o + 8) = f & ~0x20;
        }
    }
}
