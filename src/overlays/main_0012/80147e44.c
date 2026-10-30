/* func_80147E44 [80147E44,80147F9C) - owner re-derivation from retail assembly (clean input set).
 * Applies func_80147928/8014799C/80147A10, builds a rotation matrix from the s16 angles of o->f20,
 * transforms o+0x24 into the scratch words D_80127090..98 (written through a 3-word struct view and
 * read back as scalars, which keeps retail's reloads), moves them to o+0x10 (non-struct stores, so they
 * conflict with the scalar reloads and keep them in order) and clears them. */
#include "psx_types.h"

typedef struct { s32 x, y, z; } V3;
typedef struct { u8 p0[0x10]; s16 f10; s16 f12; s16 f14; } S;
typedef struct {
    u8 p0[0x10];
    s32 f10;
    s32 f14;
    s32 f18;
    u8 p1C[4];
    S *f20;
} O;

/* callers read v0: none-direct (no jal caller); void.
 * func_80147928 receives a1 unchanged (retail passes the incoming a1 through), so it is a parameter here. */
extern void func_80147928(O *o, s32 a);
extern void func_8014799C(O *o, s32 a);
extern void func_80147A10(O *o, s32 a);
extern void func_80013F3C(void *m);
extern void func_8004A0DC(s32 angle, void *m);
extern void func_80049F3C(s32 angle, void *m);
extern void func_8004A27C(s32 angle, void *m);
extern void func_800484EC(void *p, void *rec, void *q);
extern s32 D_80127090;
extern s32 D_80127094;
extern s32 D_80127098;

void func_80147E44(O *o, s32 a, s32 b, s32 c) {
    s32 v[4];
    s32 m[8];
    S *s;
    V3 *p;

    s = o->f20;
    p = (V3 *)&D_80127090;
    func_80147928(o, a);
    func_8014799C(o, b);
    func_80147A10(o, c);
    func_80013F3C(m);
    func_8004A0DC(s->f12, m);
    func_80049F3C(s->f10, m);
    func_8004A27C(s->f14, m);
    func_800484EC(m, (u8 *)o + 0x24, v);
    p->x = v[0];
    p->y = v[1];
    p->z = v[2];
    *(s32 *)((u8 *)o + 0x10) = D_80127090;
    *(s32 *)((u8 *)o + 0x14) = D_80127094;
    *(s32 *)((u8 *)o + 0x18) = D_80127098;
    D_80127090 = 0;
    D_80127094 = 0;
    D_80127098 = 0;
}
