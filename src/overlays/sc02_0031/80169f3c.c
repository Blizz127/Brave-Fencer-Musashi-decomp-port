/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatch-table handler (main_0012.s D_80181460 .word slot). Nudges o toward the camera
 * (func_800D1FC8), then allocates a sub-record (func_800D21C4 against D_80187D00); on
 * failure, cleans up via func_80146C3C and returns. On success, stores it in o->f20,
 * inits several fields (f1A/f18=0x2000, f24..f26=0x60), ORs a flag into f4 and runs
 * func_80128EA8(p, o+0x24, D_80187CC0), then bumps o->f2. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    u16 f18;
    u16 f1A;
    u8 pad1C[0x8];
    u8 f24;
    u8 f25;
    u8 f26;
} Rec;

typedef struct {
    u8 pad0[0x2];
    u16 f2;
    u8 pad4[0x1C];
    Rec *f20;
} Obj;

extern void func_800D1FC8(void *o, u16 sh);
extern Rec *func_800D21C4(void *o, u8 *a, s32 n);
extern void func_80128EA8(void *o, void *d, void *s);
extern void func_80146C3C(void *o);
extern u8 D_80187D00[];
extern u8 D_80187CC0[];

#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_80169F3C(Obj *o)
{
    Rec *p;

    func_800D1FC8(o, 6);

    p = func_800D21C4(o, D_80187D00, 0x16);
    if (p == 0) {
        goto fail;
    }

    o->f20 = p;
    p->f1A = 0x2000;
    p->f18 = 0x2000;
    p->f26 = 0x60;
    p->f25 = 0x60;
    p->f24 = 0x60;

    *(u32 *)((u8 *)p + 4) |= 0x50000000;
    func_80128EA8(p, (u8 *)o + 0x24, D_80187CC0);

    o->f2 += 1;
    RETURN_NO_VALUE;

fail:
    func_80146C3C(o);
    RETURN_NO_VALUE;
}
