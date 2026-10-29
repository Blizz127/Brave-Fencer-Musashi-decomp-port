/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x10];
    u16 f10;      /* 0x10 */
} Sub;

typedef struct {
    u8 pad0[0x20];
    Sub *f20;     /* 0x20 */
    u8 pad24[0x234 - 0x24];
    s32 f234;     /* 0x234 */
} Obj;

extern s32 D_801B6440;
extern void func_80147324(s32 a0);
extern void func_80154274(Obj *o, void *a1);
extern void func_80154A74(Obj *o, s32 a1);
extern u8 D_800D58D8[];
extern void func_801474EC(Obj *o);
extern void func_801474D8(Obj *o);
extern void func_801472C8(Obj *o);
extern void func_80146CA0(Obj *o);

void func_80158BB0(Obj *o) {
    Sub *s1;

    if (D_801B6440 == 0) {
        D_801B6440 = 1;
    }
    func_80147324(0x456);
    func_80154274(o, D_800D58D8);
    func_80154A74(o, 0x16);
    s1 = o->f20;
    o->f234 = 0xE30000;
    s1->f10 = 0;
    func_801474EC(o);
    func_801474D8(o);
    func_801472C8(o);
    func_80146CA0(o);
}
