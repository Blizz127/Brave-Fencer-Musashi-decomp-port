/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x98];
    s16 h98;
    u8 pad9A[0x1A];
    u32 flagsB4;
    u8 padB8[9];
    u8 fC1;
    u8 fC2;
} O;

extern void func_80131B14(O *o, s32 a1);
extern void func_8012B23C(O *o);
extern s32 func_80131A34(O *o, s32 a);
extern void func_8002D4C8(s32 a, s32 b);
extern s32 func_80131CA8(O *o, s32 a);

/* a1 is forwarded untouched to func_80131B14 (retail reads entry $a1). */
/* Returns retail's leftover v0: func_80131CA8's result from the final call. */
s32 func_8012FC30(O *o, s32 a1) {
    o->fC1 = 10;
    if (o->flagsB4 & 0x400) {
        func_80131B14(o, a1);
        o->fC2 = 0;
        o->h98 = 0;
        func_8012B23C(o);
        func_80131A34(o, 8);
        func_8002D4C8(0xA74, 0);
    }
    return func_80131CA8(o, 0x1A);
}