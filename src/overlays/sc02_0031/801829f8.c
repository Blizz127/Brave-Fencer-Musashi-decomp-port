/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x36];
    u16 f36;      /* 0x36 */
} Sub;

typedef struct {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
    u8 pad4[0x20 - 0x4];
    s32 f20;      /* 0x20 */
    u8 pad24[0x58 - 0x24];
    s32 f58;      /* 0x58 */
    u16 f5C;      /* 0x5C */
    u8 pad5E[0x64 - 0x5E];
    Sub *f64;     /* 0x64 */
    u8 pad68[0xFC - 0x68];
    u16 fFC;      /* 0xFC */
} Obj;

extern void *func_8012C1B8(void);
extern void func_8012CAE4(Obj *o);
extern void func_8001C214(void *a0, s32 a1);
extern u8 D_80188FD0[];

void func_801829F8(Obj *o) {
    void *r = func_8012C1B8();

    o->f20 = (s32)r;
    if (r == 0) {
        func_8012CAE4(o);
        return;
    }
    func_8001C214(r, 0);
    o->f5C = 0xCC00;
    {
        s32 flags = (s32)D_80188FD0 | 0x40000000 | 0x20000000;
        u16 x = o->f64->f36;

        o->f58 = flags;
        o->f2 = 1;
        o->fFC = x;
    }
}
