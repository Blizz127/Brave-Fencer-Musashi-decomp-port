/* Owner re-derivation from retail assembly (clean input set). */
/* Death/despawn step: sets o->f44 bit 2 and runs func_80147A10(o, 0x400); clamps
 * o->f2C to 0 when it is not negative; runs func_80147A84(o), func_801473EC(o),
 * func_80148038(o, 0x1A000), func_80147460(o); runs func_80146D90(o) when
 * func_8014ED28(o) is nonzero; runs func_80161450(o) and, when its result & 5 is
 * nonzero, runs func_80157A8C(o), func_80146E90(o, 8) and func_80146CA0(o). If that
 * result is nonzero, done. Otherwise runs func_80155FF8(o, o->f1AA); when o->f1C0 is
 * nonzero, runs func_80154A74(o, 0x17), func_80154274(o, &D_800D5178),
 * func_80146DB8(o, &D_80186F6C), func_801477E8(o, 0xFFF40000) and sets o->f2 = 4;
 * either way finishes with func_801725A4(o). void, reached only via jalr table
 * D_8018068C; the sole dispatcher func_80157808 never reads v0 after the jalr. */
#include "psx_types.h"

typedef struct Obj {
    u8 pad0[2];
    u16 f2;     /* 0x2 */
    u8 pad4[0x2C - 4];
    s32 f2C;    /* 0x2C */
    u8 pad30[0x44 - 0x30];
    u32 f44;    /* 0x44 */
    u8 pad48[0x1AA - 0x48];
    u8 f1AA;    /* 0x1AA */
    u8 pad1AB[0x1C0 - 0x1AB];
    u8 f1C0;    /* 0x1C0 */
} Obj;

extern u8 D_800D5178;
extern u8 D_80186F6C;

extern void func_80147A10(Obj *o, s32 a1);
extern void func_80147A84(Obj *o);
extern void func_801473EC(Obj *o);
extern void func_80148038(Obj *o, s32 a1);
extern void func_80147460(Obj *o);
extern s32 func_8014ED28(Obj *o);
extern void func_80146D90(Obj *o);
extern s32 func_80161450(Obj *o);
extern void func_80157A8C(Obj *o);
extern void func_80146E90(Obj *o, s32 a1);
extern void func_80146CA0(Obj *o);
extern void func_80155FF8(Obj *o, s32 a1);
extern void func_80154A74(Obj *o, s32 a1);
extern void func_80154274(Obj *o, void *a1);
extern void func_80146DB8(Obj *o, void *a1);
extern void func_801477E8(Obj *o, s32 a1);
extern void func_801725A4(Obj *o);

void func_8015795C(Obj *o)
{
    s32 s1;
    s32 unused[4];

    o->f44 |= 2;
    func_80147A10(o, 0x400);

    if (o->f2C >= 0) {
        o->f2C = 0;
    }

    func_80147A84(o);
    func_801473EC(o);
    func_80148038(o, 0x1A000);
    func_80147460(o);

    if (func_8014ED28(o) != 0) {
        func_80146D90(o);
    }

    s1 = func_80161450(o);
    if (s1 & 5) {
        func_80157A8C(o);
        func_80146E90(o, 8);
        func_80146CA0(o);
    }

    if (s1 != 0) {
        return;
    }

    func_80155FF8(o, o->f1AA);
    if (o->f1C0 != 0) {
        func_80154A74(o, 0x17);
        func_80154274(o, &D_800D5178);
        func_80146DB8(o, &D_80186F6C);
        func_801477E8(o, 0xFFF40000);
        o->f2 = 4;
    }
    func_801725A4(o);
}
