/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u16 f0;       /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8;      /* 0xB8 */
};

extern void func_801466F0(s32 a0, Obj *a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7);
extern Fn D_8018730C[];
extern void func_80147078(Obj *o, s32 a1);
extern void func_80159B70(Obj *o);
extern void func_80161208(Obj *o);

void func_8015F89C(Obj *o) {
    u16 flags = o->fB8;

    if (!(flags & 0x4000)) {
        if (flags & 0x8000) {
            D_8018730C[o->f0](o);
            func_80147078(o, 0);
            func_80159B70(o);
            return;
        }
    } else {
        func_801466F0(1, o, 0, -0x1E, 0, 0, 0, 0);
    }
    func_80161208(o);
}
