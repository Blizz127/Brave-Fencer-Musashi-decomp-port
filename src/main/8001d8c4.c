/* Owner re-derivation from retail assembly (clean room, clean input set). */
/* Builds the four corners of a sprite part's rectangle (object position + part offset, part width/height),
 * transforms each corner with the GTE rotation matrix and translation (MVMVA, RTV0TR) and stores the
 * resulting screen x/y into the four vertex slots of the part's textured quad (POLY_FT4). */
#include "psx_types.h"
#include "gte_ops.h"

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} SVec;

typedef struct {
    u8 pad0[0x2E];
    u16 x;
    u16 y;
    u16 z;
} Obj;

typedef struct {
    u16 flags;
    u8 w;
    u8 h;
    s16 u;
    u16 v;
    u16 ox;
    u16 oy;
} Part;

typedef struct {
    u32 tag;
    u8 r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad2;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad3;
} PolyFT4;

void func_8001D8C4(Obj *o, Part *p, PolyFT4 *q)
{
    SVec v[4];
    s32 m[3];
    s32 x;
    s32 y;

    x = o->x + p->ox;
    y = o->y + p->oy;
    v[0].vx = v[2].vx = x;
    v[0].vy = v[1].vy = y;
    v[1].vx = v[3].vx = x + p->w;
    v[2].vy = v[3].vy = y + p->h;
    v[0].vz = v[1].vz = v[2].vz = v[3].vz = o->z;

    gte_lwc2(0, 0, &v[0]);
    gte_lwc2(1, 4, &v[0]);
    gte_nop();
    gte_nop();
    gte_mvmva(1, 0, 0, 0, 0);
    gte_swc2(25, 0, &m[0]);
    gte_swc2(26, 4, &m[0]);
    gte_swc2(27, 8, &m[0]);
    q->x0 = m[0];
    q->y0 = m[1];

    gte_lwc2(0, 0, &v[1]);
    gte_lwc2(1, 4, &v[1]);
    gte_nop();
    gte_nop();
    gte_mvmva(1, 0, 0, 0, 0);
    gte_swc2(25, 0, &m[0]);
    gte_swc2(26, 4, &m[0]);
    gte_swc2(27, 8, &m[0]);
    q->x1 = m[0];
    q->y1 = m[1];

    gte_lwc2(0, 0, &v[2]);
    gte_lwc2(1, 4, &v[2]);
    gte_nop();
    gte_nop();
    gte_mvmva(1, 0, 0, 0, 0);
    gte_swc2(25, 0, &m[0]);
    gte_swc2(26, 4, &m[0]);
    gte_swc2(27, 8, &m[0]);
    q->x2 = m[0];
    q->y2 = m[1];

    gte_lwc2(0, 0, &v[3]);
    gte_lwc2(1, 4, &v[3]);
    gte_nop();
    gte_nop();
    gte_mvmva(1, 0, 0, 0, 0);
    gte_swc2(25, 0, &m[0]);
    gte_swc2(26, 4, &m[0]);
    gte_swc2(27, 8, &m[0]);
    q->x3 = m[0];
    q->y3 = m[1];
}
