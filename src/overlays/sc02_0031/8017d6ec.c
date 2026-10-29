/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[2];
    u16 f2;       /* 0x2 */
    u8 pad4[0x1C - 0x4];
    s32 f1C;      /* 0x1C */
    u8 pad20[0x2A - 0x20];
    u16 f2A;      /* 0x2A */
    u16 f2C;      /* 0x2C */
    u8 pad2E[0x34 - 0x2E];
    void *f34;    /* 0x34 */
} Obj;

extern void func_80147324(u16 a0);
extern void func_80147084(void *a0);
extern void func_801472B4(void *a0);
extern void func_8017D2F4(Obj *o);

void func_8017D6EC(Obj *o) {
    void *s1 = o->f34;
    s32 cur = o->f1C;

    o->f1C = cur + 1;
    if (cur < 0x20) {
        o->f2A = o->f2A - 0x10;
    } else {
        func_80147324(o->f2C);
        func_80147084(s1);
        func_801472B4(s1);
        o->f1C = 0;
        o->f2 = o->f2 + 1;
    }
    func_8017D2F4(o);
}
