/* Owner re-derivation from retail assembly (clean input set). */
/* Resets an object's action: runs func_80146DB8 with the table D_80180980, picks the pair of
 * animation requests by flag 0x200 of the object's word at 0x44 (5/0x1C when set, 4/0x11 otherwise),
 * clears the two words at 0x234/0x238 and runs the follow-up steps. */
#include "psx_types.h"

/* Object header fields used here. */
typedef struct {
    u8 pad00[0x44];
    u32 flags;
    u8 pad48[0x234 - 0x48];
    s32 f234;
    s32 f238;
} Obj;

extern u8 D_80180980[];
extern void func_80146DB8(Obj *o, u8 *table);
extern void func_80154150(Obj *o, s32 i);
extern void func_80154A74(Obj *o, s32 i);
extern void func_801553C0(Obj *o);
extern void func_801553A8(Obj *o);
extern void func_80172590(Obj *o);
extern void func_80146CA0(Obj *o);

void func_80159BE4(Obj *o)
{
    func_80146DB8(o, D_80180980);
    if (o->flags & 0x200) {
        func_80154150(o, 5);
        func_80154A74(o, 0x1C);
    } else {
        func_80154150(o, 4);
        func_80154A74(o, 0x11);
    }
    o->f234 = 0;
    o->f238 = 0;
    func_801553C0(o);
    func_801553A8(o);
    func_80172590(o);
    func_80146CA0(o);
}
