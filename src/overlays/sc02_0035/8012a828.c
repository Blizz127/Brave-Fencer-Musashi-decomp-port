/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ u8 pad00[0x72];
    /* 0x72 */ u16 flags;
    /* 0x74 */ u8 pad74[0x1C];
    /* 0x90 */ u32 f90;
    /* 0x94 */ u32 f94;
    /* 0x98 */ u8 pad98[2];
    /* 0x9A */ u16 f9A;
    /* 0x9C */ u32 f9C;
} Obj;

extern void func_8012AAAC(Obj *o);

/* Sets f90 = a, clears f94/f9A/f9C and flag bits 0x600, then func_8012AAAC(o). Retail int function, no return value. */
s32 func_8012A828(Obj *o, u32 a)
{
    o->f90 = a;
    o->f94 = 0;
    o->f9C = 0;
    o->f9A = 0;
    o->flags &= 0xF9FF;
    func_8012AAAC(o);
#ifndef __mips__
    return 0;
#endif
}
