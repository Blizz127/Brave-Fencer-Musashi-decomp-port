/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"
typedef struct {
    /* 0x00 */ s16 a;
    /* 0x02 */ s16 b;
    /* 0x04 */ s16 c;
    /* 0x06 */ s16 d;
    /* 0x08 */ s32 e;
    /* 0x0C */ s32 f;
    /* 0x10 */ s32 g;
} Req8014C010;

extern s32 func_80146AFC(Req8014C010 *r);
/* Returns retail's leftover v0: func_80146AFC's result. */
s32 func_8014C010(s32 a, s32 b)
{
    Req8014C010 r;

    r.a = 2;
    r.b = 0;
    r.c = 0;
    r.d = 0;
    r.e = a;
    r.f = b;
    r.g = 0;
    return func_80146AFC(&r);
}
