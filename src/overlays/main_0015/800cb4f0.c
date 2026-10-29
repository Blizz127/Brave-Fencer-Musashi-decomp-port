/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Spawns the beam child (func_801465E4), copies the parent's matrix with a local offset, randomises its spin and starts the beam (state + 1); without a child, func_80146C3C. */
#include "psx_types.h"

typedef struct { s16 vx, vy, vz, pad; } SVec;
typedef struct { s16 m[3][3]; s16 pad; s32 t[3]; } Mat;
typedef struct { u16 lo; s16 hi; } Fx;

typedef struct {
    u8 pad0[0x34];
    Mat m;              /* 0x34 */
} Node;

typedef struct {
    u8 pad0[0x20];
    u32 node;           /* 0x20: guest address of a Node */
} Parent;

typedef struct {
    u8 pad0[0x4];
    u32 flags;          /* 0x4 */
    u8 pad8[0x20 - 0x8];
    u32 shape;          /* 0x20: guest address */
    u8 pad24[0x34 - 0x24];
    Mat m;              /* 0x34 */
} Child;

typedef struct {
    u16 pad0;
    u16 state;          /* 0x2 */
    Fx pos[3];          /* 0x4 */
    u8 pad10[0x20 - 0x10];
    u32 child;          /* 0x20: guest address of a Child */
    u8 pad24[0x30 - 0x24];
    u32 level;          /* 0x30 */
    u32 parent;         /* 0x34: guest address of a Parent */
    Mat m;              /* 0x38 */
} Obj;

extern s32 D_800CC870[];
extern u8 D_800636C8[];
extern void *func_801465E4(void);
extern void func_8001C810(void *p, s32 *list);
extern void func_80149350(void *obj, s32 a1, s32 a2);
extern void func_80149374(void *o, void *x);
extern void func_80017DC4(s16 *src, s16 *dst);
extern int func_8005C49C(void);
extern void func_8004A27C(s32 r, void *m);
extern void func_8004818C(void *a, void *b, void *c);
extern s32 func_80146A6C(s32 a, void *o, s32 c, s32 d, s32 e, s32 f, s32 g);
extern void func_800CB6FC(Obj *o);
extern void func_80146C3C(void *o);

void func_800CB4F0(Obj *o)
{
    Parent *parent = (Parent *)o->parent;
    Child *c;
    SVec v;
    Mat r;

    c = func_801465E4();
    o->child = (u32)c;
    if (c != 0) {
        func_8001C810(c, D_800CC870);
        c->shape = (u32)D_800636C8;
        c->flags |= 0x50000040;
        v.vx = 0;
        v.vy = -6;
        v.vz = o->pos[2].hi - 0x24;
        func_80149350(parent, (s32)&v, (s32)&v);
        o->m = ((Node *)parent->node)->m;
        o->m.t[0] = v.vx;
        o->m.t[1] = v.vy;
        o->m.t[2] = v.vz;
        func_80149374(parent, o->pos);
        v.vx = v.vy = v.vz = 0x1800;
        func_80017DC4((s16 *)&v, (s16 *)&r);
        func_8004A27C(func_8005C49C() & 0xFFF, &r);
        func_8004818C(&((Node *)parent->node)->m, &r, &c->m);
        func_80146A6C(0x13, o, o->pos[0].hi, o->pos[1].hi, o->pos[2].hi, 0, 0);
        o->level = 0x80;
        func_800CB6FC(o);
        o->state++;
    } else {
        func_80146C3C(o);
    }
}
