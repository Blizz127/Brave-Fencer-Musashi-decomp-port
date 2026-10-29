/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up the grid cell under the point D_8017E910 in a cell map, walks that cell's entry list backwards
 * and tests each entry (plain or flagged) with func_801347A0; on a hit stores the entry's first u16 in
 * D_80184130 and returns 1. Returns 0 if the point is outside the grid or nothing hits. */
#include "psx_types.h"

/* Collision plane (layout as in 801347a0.c). */
typedef struct {
    s16 v[3];
    s16 w;
} Plane;

/* Cell map descriptor; pointer fields are guest addresses. */
typedef struct {
    u16 x0;
    u16 z0;
    u16 w;
    u16 h;
    u32 cells;   /* u16 pairs {offset, count} per cell */
    u32 tbl;     /* s32 * : per-plane table (callee arg 4) */
    u32 planes;  /* Plane * : plane array (callee arg 3) */
    u32 lists;   /* base of the u16 entry lists */
    u32 plain;   /* base of plain entries (18 bytes) */
    u32 flagged; /* base of flagged entries (22 bytes) */
} CellMap;

extern u32 D_8017E910; /* guest u16 * : the point */
extern u16 D_80184130;

extern s32 func_801347A0(s16 flag, s16 *p, Plane *base, s32 *tbl);

int func_801345F8(CellMap *m)
{
    u16 *pt = (u16 *)D_8017E910;
    u16 *c;
    u16 *e;
    u16 n;
    u32 rec;
    s16 flag;
    u16 v;
    u16 dx, dz;
    int ex, ez;
    u32 tbl, planes, plain, flagged, lists;

    ex = ((pt[0] + 0x8000) >> 7 & 0x1FF) - m->x0;
    dx = ex;
    if ((u16)ex >= m->w) {
        return 0;
    }
    ez = ((pt[2] + 0x8000) >> 7 & 0x1FF) - m->z0;
    dz = ez;
    if ((u16)ez >= m->h) {
        return 0;
    }
    tbl = m->tbl;
    planes = m->planes;
    plain = m->plain;
    flagged = m->flagged;
    c = (u16 *)((u16)(((dz & 0xFFFF) * m->w + (dx & 0xFFFF)) * 2) * 2 + m->cells);
    lists = m->lists;
    n = c[1];
    e = (u16 *)(lists + c[0]) + n - 1;
    while (n--) {
        v = *e;
        flag = v & 0x8000;
        if (flag == 0) {
            rec = plain + v * 18;
        } else {
            rec = flagged + (v & 0x7FFF) * 22;
        }
        if (func_801347A0(flag, (s16 *)rec, (Plane *)planes, (s32 *)tbl)) {
            D_80184130 = *(u16 *)rec;
            return 1;
        }
        e--;
    }
    return 0;
}
