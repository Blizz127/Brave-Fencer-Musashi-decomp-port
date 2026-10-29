/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    s32 f18;      /* 0x18 */
    s32 f1C;      /* 0x1C */
    s32 f20;      /* 0x20 */
    u8 pad24[0x50 - 0x24];
    u16 f50;      /* 0x50 */
    u16 f52;      /* 0x52 */
    u16 f54;      /* 0x54 */
} Obj;

extern void func_80052D90(s32 a0, Obj *a1);
extern void func_80054514(Obj *a0, void *a1);

void func_8017F53C(Obj *o) {
    s32 tmp[8];

    func_80052D90(0, o);
    func_80054514(o, tmp);
    o->f18 = 0;
    o->f1C = 0;
    o->f20 = 0;
    o->f50 = 0;
    o->f52 = 0;
    o->f54 = 0;
}
