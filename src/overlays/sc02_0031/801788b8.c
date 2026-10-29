/* Owner re-derivation from retail assembly (clean input set). */
/* Spawns effect object 0x83 for o (model from +0x70), links it back to o, places it at o's position and * yaw, runs its setup and stores `arg` at +0xDC. Returns the new object or NULL. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    u16 yaw; /* 0x12 */
} Sub801788B8;

typedef struct {
    u8 pad0[6];
    u16 x;     /* 0x06 */
    u8 pad8[2];
    u16 y;     /* 0x0A */
    u8 padC[2];
    u16 z;     /* 0x0E */
    u8 pad10[0x10];
    u32 sub;   /* 0x20: guest address of Sub801788B8 */
    u8 pad24[0x4C];
    s16 model; /* 0x70 */
} Obj801788B8;

typedef struct {
    u8 pad0[0x64];
    u32 owner; /* 0x64 */
    u8 pad68[0x74];
    u32 arg;   /* 0xDC */
} Eff801788B8;

extern Eff801788B8 *func_8012C658(s32 a, s32 b, s32 c);
extern void func_8012E8E0(Eff801788B8 *e, s16 *pos);
extern void func_8012B2CC(Eff801788B8 *e);

Eff801788B8 *func_801788B8(Obj801788B8 *o, u32 arg)
{
    Eff801788B8 *e;
    Sub801788B8 *sub;
    s16 pos[4];

    e = func_8012C658(0x83, o->model, 0);
    if (e == 0) {
        return 0;
    }
    e->owner = (u32)o;
    pos[0] = o->x;
    pos[1] = o->y;
    pos[2] = o->z;
    sub = (Sub801788B8 *)o->sub;
    if (sub != 0) {
        pos[3] = sub->yaw;
    } else {
        pos[3] = 0;
    }
    func_8012E8E0(e, pos);
    func_8012B2CC(e);
    e->arg = arg;
    return e;
}
