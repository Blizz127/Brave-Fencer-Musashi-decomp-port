/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

extern u16 *func_8001D074(s32 a, s32 b);
extern u8 *func_801291C0(void);
extern void func_8001CC3C(u16 *h, s32 a, s32 b, s32 c);

u8 *func_8012913C(u16 id) {
    u16 *h;
    u8 *r;
    h = func_8001D074(0x7E, 0x100);
    if (h == 0) {
        return 0;
    }
    r = func_801291C0();
    if (r == 0) {
        return 0;
    }
    *(u16 *)r = id;
    *(u32 *)(r + 0x20) = (u32)h;
    func_8001CC3C(h, 0, 0, 0);
    return r;
}
