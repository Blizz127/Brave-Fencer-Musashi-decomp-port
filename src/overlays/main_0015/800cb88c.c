/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Builds the beam matrix from the object's position (func_800D20C0/func_800D23D0, then translation and rotation), sets level 0xC0, draws and advances the state. */
#include "psx_types.h"

typedef struct { s16 vx, vy, vz, pad; } SVec;
typedef struct { u16 lo; s16 hi; } Fx;

typedef struct {
    u16 pad0;
    u16 state;          /* 0x2 */
    Fx pos[3];          /* 0x4 */
    u8 pad10[0x30 - 0x10];
    u32 level;          /* 0x30 */
    u8 pad34[0x38 - 0x34];
    u8 m[0x20];         /* 0x38: matrix */
} Obj;

extern void func_800D20C0(void *pos, void *out, u16 sh);
extern void func_800D23D0(void *v);
extern void func_80017E68(void *a, void *b);
extern void func_80049A1C(void *a, void *b);
extern void func_800CB988(Obj *o);

void func_800CB88C(Obj *o)
{
    SVec pos;
    SVec rot;

    pos.vx = o->pos[0].hi;
    pos.vy = o->pos[1].hi;
    pos.vz = o->pos[2].hi;
    func_800D20C0(&pos, &rot, 7);
    func_800D23D0(&rot);
    func_80017E68(&pos, o->m);
    func_80049A1C(&rot, o->m);
    o->level = 0xC0;
    func_800CB988(o);
    o->state++;
}
