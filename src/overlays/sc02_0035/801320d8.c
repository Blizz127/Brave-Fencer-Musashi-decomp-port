/* Owner re-derivation from retail assembly (clean input set). */
/* Stores the pointer from func_8012C1B8() into f20; if non-null, initialises it via func_8001C214,
 * points f58 at D_80184A00, sets f5C = 0x80 and bumps f2; otherwise calls func_8012CAE4(o). Retail
 * leaves whatever is in v0 at the end of each arm, no explicit return value. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x2];
    u16 f2;
    u8 pad4[0x1C];
    u32 f20; /* guest pointer */
    u8 pad24[0x34];
    u32 f58; /* guest pointer */
    u16 f5C;
} Obj;

extern void *func_8012C1B8(void);
extern void func_8012CAE4(void *o);
extern void func_8001C214(void *p, s32 *list);
extern u8 D_80184A00;

s32 func_801320D8(Obj *o)
{
    void *p;
    u16 t;

    p = func_8012C1B8();
    o->f20 = (u32)p;
    if (p == 0) {
        func_8012CAE4(o);
    } else {
        func_8001C214(p, 0);
        o->f58 = (u32)&D_80184A00;
        t = o->f2;
        o->f5C = 0x80;
        o->f2 = t + 1;
    }
#ifndef __mips__
    return 0;
#endif
}
