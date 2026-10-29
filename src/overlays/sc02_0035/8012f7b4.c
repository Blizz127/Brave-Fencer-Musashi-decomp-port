/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1C];
    s32 timer;
    u8 pad20[0x94];
    u32 flagsB4;
} O;

extern void func_8012BEE8(O *o);
extern s32 func_80131A34(O *o, s32 a);
extern void func_80131CA8(O *o, s32 a);

void func_8012F7B4(O *o) {
    if (o->flagsB4 & 4) {
        func_8012BEE8(o);
        if (o->timer == 1 && func_80131A34(o, 8) == 0) {
            func_80131A34(o, 0x20);
        }
    }
    func_80131CA8(o, 7);
}