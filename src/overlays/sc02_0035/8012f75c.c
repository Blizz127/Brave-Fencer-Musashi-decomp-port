/* Owner re-derivation from retail assembly (clean input set). */
/* Sets +0xC1 to 3; when flag 4 of +0xB4 is set runs func_80131B14, sets the +0x1C timer to 0x10 and clears +0x98; then func_80131CA8(o,6). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1C];
    s32 timer;
    u8 pad20[0x78];
    s16 f98;
    u8 pad9A[0x1A];
    u32 flagsB4;
    u8 padB8[9];
    u8 fC1;
} Obj;

extern void func_80131B14(Obj *o, s32 a1);
extern s32 func_80131CA8(Obj *o, s32 a);

/* a1 is forwarded untouched to func_80131B14 (retail reads entry $a1). */
/* Returns retail's leftover v0: func_80131CA8's result from the final call. */
s32 func_8012F75C(Obj *o, s32 a1) {
    o->fC1 = 3;
    if (o->flagsB4 & 4) {
        func_80131B14(o, a1);
        o->timer = 0x10;
        o->f98 = 0;
    }
    return func_80131CA8(o, 6);
}
