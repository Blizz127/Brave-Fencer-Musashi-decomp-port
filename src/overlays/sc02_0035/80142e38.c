/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    u16 f18; /* 0x18 */
    u16 f1A; /* 0x1A */
} Rec;

typedef struct {
    u8 pad0[0x20];
    Rec *f20;  /* 0x20 */
    u8 pad24[0x70 - 0x24];
    u16 f70;   /* 0x70 */
    u8 pad72[0xFC - 0x72];
    u16 fFC;   /* 0xFC */
} Obj;

extern Rec *func_8012C1B8(Obj *o);
extern void func_8012CAE4(Obj *o);
extern void func_8001CA1C(u32 e, u8 *tbl);
extern void func_80142B2C(Obj *o);
extern u8 D_80185928[];

void func_80142E38(Obj *o) {
    Rec *v0val;
    Rec *rec;

    v0val = func_8012C1B8(o);
    rec = v0val;
    o->f20 = v0val;
    if (v0val == 0) {
        func_8012CAE4(o);
        return;
    }

    func_8001CA1C((u32)rec, D_80185928);
    {
        u16 v1;
        u16 v0;

        v1 = o->f70;
        v1 = v1 & 0xF;
        v0 = v1 * 3;
        v0 = v0 << 9;
        v0 = v0 + 0x1200;
        rec->f1A = v0;
        rec->f18 = v0;
    }
    o->fFC = 0;
    func_80142B2C(o);
}
