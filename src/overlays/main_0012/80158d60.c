/* func_80158D60 [80158D60,80158E24) - owner re-derivation from retail assembly (clean input set). */
/* Steps the object once (four times when func_801487F4 reports bits 0xF0); when the child's
 * +0x10 height drops below 0xC00, plays sound 0x454, clamps it, sets +0x238 and stops. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x10];
    s16 h10; /* 0x10 */
} Sub80158D60;

typedef struct {
    u8 pad00[0x20];
    Sub80158D60 *sub; /* 0x20 */
    u8 pad24[0x214];
    u32 f238;         /* 0x238 */
} Obj80158D60;

/* callers read v0: no direct jal callers; retail sets no v0. */
extern u16 func_801487F4(Obj80158D60 *o);
extern void func_80159434(Obj80158D60 *o, s32 d);
extern void func_80147324(s32 id);
extern void func_80146CA0(void *o);

void func_80158D60(Obj80158D60 *o)
{
    s32 n;
    s32 d;

    d = 0x4C000;
    n = 1;
    if ((func_801487F4(o) & 0xF0) != 0) {
        n = 4;
    }
    for (; n > 0; n--) {
        func_80159434(o, d);
        if (o->sub->h10 < 0xC00) {
            func_80147324(0x454);
            o->sub->h10 = 0xC00;
            o->f238 = 0xFFEA0000;
            func_80146CA0(o);
            n = 0;
        }
    }
}
