/* Owner re-derivation from retail assembly (clean input set). */
/* Double-buffer step: tries func_800D27A0(o, arg) up to 0x20 times; on the first nonzero handle flips the buffer
 * index at +8, hands the handle and the newly selected buffer word to func_800D3234, calls func_80046B44 with the
 * handle and returns 0; returns 1 if every try failed. */
#include "psx_types.h"

typedef struct {
    u32 buf[2];
    u32 cur;
} DblBuf;

extern u32 func_800D27A0(DblBuf *o, u32 arg);
extern void func_800D3234(u32 h, u32 buf);
extern void func_80046B44(u32 h);

s32 func_800D2704(DblBuf *o, u32 arg)
{
    s32 n;
    u32 h;

    for (n = 0x20; n != 0; n--) {
        h = func_800D27A0(o, arg);
        if (h != 0) {
            o->cur = (o->cur == 0);
            func_800D3234(h, o->buf[o->cur]);
            func_80046B44(h);
            return 0;
        }
    }
    return 1;
}
