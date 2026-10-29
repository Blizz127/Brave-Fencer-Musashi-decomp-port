/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 f0;        /* 0x0 */
    u8 pad1[1];
    u8 f2;        /* 0x2 */
    u8 pad3[1];
    u16 f4;       /* 0x4 */
} Obj;

extern u16 D_80188030[];
extern void func_8016EC0C(Obj *o, s32 a1);

void func_8016EBA8(Obj *o) {
    u8 oldf2;

    o->f4 = o->f4 - D_80188030[o->f2];
    func_8016EC0C(o, 0x80);
    oldf2 = o->f2;
    o->f2 = oldf2 + 0xFF;
    if (oldf2 == 0) {
        o->f0 = 0;
    }
}
