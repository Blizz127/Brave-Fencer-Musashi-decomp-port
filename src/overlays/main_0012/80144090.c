/* Owner re-derivation from retail assembly (clean input set). */
/* Integrates the object's position, projects it, and emits a 16-triangle fan (radius from +0x1C) into the ordering table. */
#include "psx_types.h"

typedef struct {
    u8 p0[3];
    u8 len;
    u32 rgbc;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;
} Tri;

typedef struct {
    s16 x;
    s16 y;
    s32 d;
    s32 f;
} R;

typedef struct {
    u32 w0;
    u8 p4[16];
} Ot;

typedef struct {
    u8 p0[4];
    s32 f4;
    s32 f8;
    s32 fC;
    s32 f10;
    s32 f14;
    s32 f18;
    s32 f1C;
} O;

extern Ot D_800A651C[];
extern u8 D_800AF648;
extern u16 D_800B9A02;
extern void *func_80010A08(s32 size);
extern s32 func_8004787C(s32 a);
extern s32 func_80047948(s32 a);
extern void func_8004914C(void *p);
extern void func_800491AC(void *p);
extern s32 func_8004955C(void *v, void *sxy, void *p, void *flag);
extern void func_80058C6C(u32 p, void *q);

void func_80144090(u8 *op) {
    O *o = (O *)op;
    Tri *q;
    s16 v[3];
    R r;
    s32 idx;
    s32 ang;
    s32 a;
    s32 b;
    q = func_80010A08(0x140);
    if (q != 0) {
        o->f4 += o->f10;
        o->f8 += o->f14;
        o->fC += o->f18;
        v[0] = *(u16 *)(op + 6);
        v[1] = *(u16 *)(op + 0xA);
        v[2] = *(u16 *)(op + 0xE);
        func_8004914C(&D_800AF648);
        func_800491AC(&D_800AF648);
        idx = func_8004955C(v, &r, &r.d, &r.f) + 4;
        if (idx < 0) {
            idx = 0;
        }
        ang = 0;
        do {
            q->rgbc = 0xFFFFFF;
            q->len = 4;
            ((u8 *)q)[7] = 0x20;
            if (o->f1C - 6 <= 0) {
                q->x0 = r.x;
                q->y0 = r.y;
            } else {
                q->x0 = r.x + ((func_8004787C(ang) * ((o->f1C - 6) << 4)) >> 12);
                q->y0 = r.y + ((func_80047948(ang) * ((o->f1C - 6) << 4)) >> 12);
            }
            a = func_8004787C(ang) * (o->f1C << 4);
            b = func_80047948(ang) * (o->f1C << 4);
            ang += 0x100;
            q->x2 = r.x + ((a + (b >> 6)) >> 12);
            q->y2 = r.y + ((b + (a >> 6)) >> 12);
            q->x1 = r.x + ((a - (b >> 6)) >> 12);
            q->y1 = r.y + ((b - (a >> 6)) >> 12);
            func_80058C6C(D_800A651C[D_800B9A02].w0 + (idx << 2), q);
            q++;
        } while (ang < 0x1000);
    }
}
