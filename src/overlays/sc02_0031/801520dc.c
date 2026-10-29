/* Owner re-derivation from retail assembly (clean input set). */
/* Same reset family as func_801528B0/func_80152DA8, with func_801472C8(o) swapped in for
 * func_80154274, its own table (D_801869A4) and clamp constant for func_801477E8, and an
 * extra func_80146A6C(0x10, o, o->f6, o->fA, o->fE, 0, 0) spawn before the final
 * func_80147324/func_80146CA0 pair. void, no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x6];
    s16 f6;       /* 0x6 */
    u8 pad8[2];
    s16 fA;       /* 0xA */
    u8 padC[2];
    s16 fE;       /* 0xE */
    u8 pad10[0xDE - 0x10];
    u8 fDE;       /* 0xDE */
    u8 padDF[0xF2 - 0xDF];
    s16 fF2;      /* 0xF2 */
} Obj;

extern u8 D_80062C04[];
extern s32 D_801869A4[];

extern void func_80019064(s32 a0);
extern void func_80151204(u8 *o, s32 b);
extern void func_80149020(Obj *o);
extern void func_801472C8(Obj *o);
extern void func_80147324(u16 a);
extern void func_80146DB8(s32 *p, s32 *q);
extern void func_801477E8(Obj *o, s32 v);
extern void func_8014CBF8(Obj *o);
extern void func_801474EC(Obj *o);
extern void func_80146A6C(s32 a, Obj *o, s16 c, s16 d, s16 e, s32 f, s32 g);
extern void func_80146CA0(Obj *o);

void func_801520DC(Obj *o)
{
    func_80019064((s32)D_80062C04);
    func_80151204((u8 *)o, o->fF2);
    func_80149020(o);
    func_801472C8(o);
    o->fDE = 0x10;
    func_80146DB8((s32 *)o, D_801869A4);
    func_801477E8(o, 0x40000);
    func_8014CBF8(o);
    func_801474EC(o);
    func_80146A6C(0x10, o, o->f6, o->fA, o->fE, 0, 0);
    func_80147324(0x441);
    func_80146CA0(o);
}
