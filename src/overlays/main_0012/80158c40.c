/* Owner re-derivation from retail assembly (clean input set). */
/* Returns retail's leftover v0 (see ledger). */
/* Runs func_80159404 once (four times if the +0xAA value has a high nibble) and aborts the repeat when the +0x234 word goes negative. */
#include "psx_types.h"
typedef struct { u8 p0[0x234]; s32 f234; } O;
extern u16 func_801487F4(O *o);
extern void func_80159404(O *o, s32 a);
extern void func_80146E90(O *o, s32 a);
extern s32 func_80146CA0(O *o);
s32 func_80158C40(O *o) {
    s32 k = 0x3C000;
    s32 n = 1;
    s32 r;
    r = func_801487F4(o) & 0xF0;
    if (r) n = 4;
    for (; n > 0; n--) {
        func_80159404(o, k);
        r = o->f234;
        if (r < 0) {
            func_80146E90(o, 4);
            r = func_80146CA0(o);
            n = 0;
        }
    }
    return r;
}
