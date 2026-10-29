/* Owner re-derivation from retail assembly (clean input set). */
/* Straight-line entity-despawn sequence: refreshes o's cached slot pointer against
 * D_800D51E0, marks state 0xE, re-syncs against D_80186F90 (indices 0x15 and the
 * func_80146DB8 triple), fires a generic sound/effect call, zeroes o->f234, releases
 * tracked id 0x455, and bumps o's counter at +2. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1AA];
    u8 f1AA;
    u8 pad1AB[0x234 - 0x1AB];
    u32 f234;
} Obj;

extern void func_80154274(void *o, u32 v);
extern void func_80154A74(void *o, s32 i);
extern void func_80146DB8(s32 *p, s32 *q);
extern void func_80146994(u16 a, s32 b, s32 c, s32 d);
extern void func_80147324(u16 a);
extern void func_80146CA0(void *o);
extern u32 D_800D51E0;
extern s32 D_80186F90[];

void func_80158038(Obj *o)
{
    o->f1AA = 0xE;
    func_80154274(o, (u32)&D_800D51E0);

    func_80154A74(o, 0x15);

    func_80146DB8((s32 *)o, D_80186F90);

    o->f234 = 0;
    func_80146994(5, (s32)o, 0xC, 0);

    func_80147324(0x455);

    func_80146CA0(o);
}
