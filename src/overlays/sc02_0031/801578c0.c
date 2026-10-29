/* Owner re-derivation from retail assembly (clean input set). */
/* Straight-line entity-despawn sequence (same shape as func_80158038): resets o via
 * func_80156648, marks state 9, re-syncs against D_800D5128/D_80186F5C, fires two
 * func_80146994 effect calls, bumps o's counter, and sets o's flags bit 2 via
 * func_801725A4. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1AA];
    u8 f1AA;
} Obj;

extern void func_80156648(void *o);
extern void func_80154274(void *o, u32 v);
extern void func_80154A74(void *o, s32 a);
extern void func_80147324(u16 a);
extern void func_80146DB8(s32 *p, s32 *q);
extern void func_80146994(u16 a, s32 b, s32 c, s32 d);
extern void func_80146CA0(void *o);
extern void func_801725A4(void *o);
extern u32 D_800D5128;
extern s32 D_80186F5C[];

void func_801578C0(Obj *o)
{
    func_80156648(o);
    o->f1AA = 9;

    func_80154274(o, (u32)&D_800D5128);

    func_80154A74(o, 0x15);

    func_80147324(0xC20);

    func_80146DB8((s32 *)o, D_80186F5C);

    func_80146994(4, (s32)o, 7, 0);
    func_80146994(5, (s32)o, 9, 0);

    func_80146CA0(o);

    func_801725A4(o);
}
