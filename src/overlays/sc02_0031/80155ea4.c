/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;

struct Obj {
    u8 pad0[0x2C];
    s32 f2C;      /* 0x2C */
    u8 pad30[0x44 - 0x30];
    s32 f44;      /* 0x44 */
    u8 pad48[0x1AA - 0x48];
    u8 f1AA;      /* 0x1AA */
    u8 pad1AB[0x1C0 - 0x1AB];
    u8 f1C0;      /* 0x1C0 */
};

extern s16 D_800B99DA;

extern void func_80147AD4(Obj *o, s32 a1, s32 a2, s32 a3);
extern void func_801473EC(Obj *o);
extern void func_8014C010(Obj *o, s32 a1);
extern void func_80155FF8(Obj *o, s32 a1);
extern void func_801599A4(Obj *o);
extern void func_80159B3C(Obj *o);
extern void func_80161208(Obj *o);

void func_80155EA4(Obj *o) {
    s32 pad[4];

    o->f44 |= 2;
    func_80147AD4(o, 0, 0, 0x8000);
    func_801473EC(o);
    if ((D_800B99DA & 3) == 0) {
        func_8014C010(o, 2);
    }
    func_80155FF8(o, o->f1AA);
    if (o->f2C >= 0 || o->f1C0 != 0) {
        func_801599A4(o);
        func_80159B3C(o);
    } else {
        func_80161208(o);
    }
}
