/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    u8 p0[0x1C];
    s32 f1C;
    u32 f20;
    u8 p24[0x38];
    s16 f5C;
    u8 p5E[0x12];
    u16 f70;
    u16 f72;
    u8 p74[0x58];
    s32 fCC;
    u8 pD0[0x30];
    s16 f100;
} O;

extern void func_80142C84(O *o);
extern s32 func_80143994(O *o, s32 a);
extern s32 func_8012AD50(O *o);

/* Returns retail's leftover v0: func_8012AD50's result from the final call. */
s32 func_80142B2C(O *o) {
    o->f1C = 0x180;
    o->f5C = 0x100;
    o->f100 = o->f70 & 0x100;
    o->f70 = o->f70 & 3;
    o->f72 = o->f72 | 0x1000;
    if (o->f100 == 0) {
        func_80142C84(o);
    }
    o->fCC = func_80143994(o, *(s16 *)(o->f20 + 0x18));
    return func_8012AD50(o);
}
