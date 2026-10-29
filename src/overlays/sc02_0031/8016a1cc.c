/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj {
    u8 pad0[0x1C];
    s32 f1C;      /* 0x1C */
    u8 pad20[0x2A - 0x20];
    s16 f2A;      /* 0x2A */
    u8 pad2C[0x30 - 0x2C];
    s32 f30;      /* 0x30 */
} Obj;

extern u8 D_80078EBF;
extern void func_80147364(s32 a0, s32 a1);
extern void func_80146C3C(Obj *o);
extern void func_80147324(s32 a0);

void func_8016A1CC(Obj *o) {
    u8 flag = D_80078EBF;
    s32 v;

    if (flag != 0) {
        v = flag - 0x18;
        o->f30 = v;
        if (v < 0) {
            return;
        }
        o->f1C++;
        if (o->f1C == 4) {
            func_80147324(0x44C);
        }

        if (D_80078EBF < 0x80) {
            return;
        }

        if (o->f2A == 0) {
            o->f2A = 1;
            o->f1C = 0x100;
        }

        if ((o->f1C & 0x1F) == 0) {
            func_80147324(0x44D);
        }
    } else {
        func_80147364(4, 0x44C);
        func_80146C3C(o);
    }
}
