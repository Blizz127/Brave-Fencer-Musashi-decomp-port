/* Owner re-derivation from retail assembly (clean input set). */
/* A straight-line teardown/reset sequence: registers &D_80062C04 as the active target
 * (func_80019064), then runs a fixed series of per-object reset calls (all already
 * matched sibling bodies), setting o->f0xDE=0x10 and passing func_80147324 a bare
 * constant (no object). void, no caller found in any region. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xDE];
    u8 fDE;       /* 0xDE */
    u8 padDF[0xF2 - 0xDF];
    s16 fF2;      /* 0xF2 */
} Obj;

extern u8 D_80062C04[];
extern u8 D_800D5880[];
extern s32 D_801869EC[];

extern void func_80019064(s32 a0);
extern void func_80151204(u8 *o, s32 b);
extern void func_80149020(Obj *o);
extern void func_80154274(Obj *o, void *p);
extern void func_80147324(u16 a);
extern void func_80146DB8(s32 *p, s32 *q);
extern void func_801477E8(Obj *o, s32 v);
extern void func_8014CBF8(Obj *o);
extern void func_801474EC(Obj *o);
extern void func_80146CA0(Obj *o);

void func_801528B0(Obj *o)
{
    func_80019064((s32)D_80062C04);
    func_80151204((u8 *)o, o->fF2);
    func_80149020(o);
    func_80154274(o, D_800D5880);
    func_80147324(0x451);
    o->fDE = 0x10;
    func_80146DB8((s32 *)o, D_801869EC);
    func_801477E8(o, -0x100000);
    func_8014CBF8(o);
    func_801474EC(o);
    func_80146CA0(o);
}
