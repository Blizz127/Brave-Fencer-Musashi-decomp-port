/* Owner re-derivation from retail assembly (clean input set). UNVERIFIED DRAFT: not compiled yet. */
/* Draws four upright beam quads from the signed byte table D_800CBB44 (x/y pairs, overlapping by one point), lit at o->level with half blue. */
#include "psx_types.h"

typedef struct { s16 vx, vy, vz, pad; } SVec;
typedef struct { u8 r, g, b, cd; } Col;

typedef struct {
    SVec v[4];          /* 0x00 */
    Col c[4];           /* 0x20 */
    u32 code;           /* 0x30 */
} Quad;

typedef struct {
    u8 pad0[0x30];
    u32 level;          /* 0x30 */
    u8 pad34[0x38 - 0x34];
    u8 m[0x20];         /* 0x38: matrix */
} Obj;

extern u8 D_800CBB44[];
extern void func_80017758(void *p, void *q);

void func_800CB988(Obj *o)
{
    Quad q;
    u8 *p;
    s16 i;
    u8 c;

    p = D_800CBB44;
    i = 0;
    q.v[1].vx = q.v[1].vy = q.v[1].vz = 0;
    q.v[0].vz = q.v[2].vz = q.v[3].vz = 0;
    c = o->level;
    q.c[0].r = q.c[0].g = q.c[0].b = 0;
    q.c[2].r = q.c[2].g = q.c[2].b = 0;
    q.c[3].r = q.c[3].g = q.c[3].b = 0;
    q.code = 0x50000000;
    q.c[1].r = q.c[1].g = c;
    q.c[1].b = c >> 1;
    for (; i < 4; i++) {
        q.v[0].vx = (s8)*p++;
        q.v[0].vy = (s8)*p++;
        q.v[2].vx = (s8)*p++;
        q.v[2].vy = (s8)*p++;
        q.v[3].vx = (s8)*p++;
        q.v[3].vy = (s8)*p--;
        func_80017758(&q, o->m);
    }
}
