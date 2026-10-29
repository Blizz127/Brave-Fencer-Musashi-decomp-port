/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
extern u8 D_80078EAE;
extern void *func_8001CFDC(s32 first, s32 limit);
extern void func_8012CAE4(u8 *o);
extern void func_8001C214(void *p, s32 a);

s32 func_8012C354(u8 *o, u8 *a)
{
    void *h;
    s32 v;
    u32 w;

    *(u8 **)(o + 0x78) = a;
    if (D_80078EAE == 0) {
        v = (*(s16 *)a * 24) / 16;
    } else {
        v = *(u16 *)a;
    }
    *(u16 *)(o + 0x76) = v;
    *(u16 *)(o + 0x5C) = *(u16 *)(a + 2);
    if (*(u32 *)(a + 4) != 0) {
        h = func_8001CFDC(0x26, 0x85);
        *(void **)(o + 0x20) = h;
        if (h == 0) {
            func_8012CAE4(o);
            return 0;
        }
        func_8001C214(h, *(s32 *)(a + 4));
    }
    w = *(u32 *)(a + 8);
    if (w != 0) {
        *(u32 *)(o + 0x58) = w | 0x60000000;
    }
    return 1;
}
