/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x18];
    u16 f18;
    u16 f1A;
    u8 pad1C[0x1A];
    u16 f36;
} Node80142608;

typedef struct {
    u8 pad00[0x20];
    u32 f20;
    u8 pad24[0x40];
    u32 f64;
    u8 pad68[8];
    s16 f70;
    u8 pad72[0x1E];
    u32 f90;
    u8 pad94[4];
    u16 f98;
    u8 pad9A[0x62];
    u16 fFC;
    u16 fFE;
} Obj80142608;

extern u8 D_8017F798[];
extern Node80142608 *func_8012C1B8(void);
extern Node80142608 *func_8012C1DC(u32 a);
extern void func_8001CA1C(void *p, void *a1);
extern void func_8012CAE4(Obj80142608 *o);
extern void func_8012AD50(Obj80142608 *o);

s32 func_80142608(Obj80142608 *o) {
    Node80142608 *p;
    Node80142608 *q;

    if (o->f70 != 0) {
        p = func_8012C1B8();
    } else {
        p = func_8012C1DC(o->f64);
    }
    o->f20 = (u32)p;
    if (p != 0) {
        func_8001CA1C(p, D_8017F798);
        p->f1A = 0x200;
        p->f18 = 0x200;
        o->fFC = 0x40;
        if (o->f70 != 0) {
            goto ok;
        }
        q = (Node80142608 *)o->f64;
        if (q != 0) {
            o->fFE = q->f36;
            goto ok;
        }
    }
    func_8012CAE4(o);
    goto done;
ok:
    o->f98 = 0;
    o->f90 = 0;
    func_8012AD50(o);
done:;
}
