/* Owner re-derivation from retail assembly (clean input set). */
/* If the global flag byte D_80078EC0 has bit 0x80 set, defers to func_80160818 and returns
 * early. Otherwise runs the normal init sequence: func_80154150(o,0x1D), sets o->f44 bit 0,
 * func_80149034(o), func_801553A8(o), stashes func_80149184(o)'s result in D_801152A0, and
 * bumps o's counter via func_80146CA0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 f44;
} Obj;

extern u8 D_80078EC0;
extern void func_80160818(void);
extern void func_80154150(void *o, s32 a);
extern void func_80149034(void *o);
extern void func_801553A8(void *o);
extern u16 func_80149184(void *o);
extern void func_80146CA0(void *o);
extern u16 D_801152A0;

void func_8015E9B8(Obj *o)
{
    if (D_80078EC0 & 0x80) {
        func_80160818();
        return;
    }
    func_80154150(o, 0x1D);
    o->f44 |= 1;
    func_80149034(o);
    func_801553A8(o);
    D_801152A0 = func_80149184(o);
    func_80146CA0(o);
}
