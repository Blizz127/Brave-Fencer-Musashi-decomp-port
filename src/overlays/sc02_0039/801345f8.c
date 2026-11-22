/* Owner re-derivation of the scene floor probe. */
#include "../main_0012/floor_cell_map.h"

extern u32 D_8017F68C; /* guest u16 * : the point */
extern u16 D_8019F9C8;

s32 func_801345F8(FloorCellMap *m)
{
    SceneProbeVector *pt = FLOOR_ADDRESS(SceneProbeVector, D_8017F68C);
    u16 *c;
    u32 cursor;
    u16 n;
    u32 rec;
    s16 flag;
    u16 v;
    u16 dx, dz;
    int ex, ez;
    u32 tbl, planes, plain, flagged, lists;

    ex = (((u16)pt->x + 0x8000) >> 7 & 0x1FF) - m->x0;
    dx = ex;
    if ((u16)ex >= m->w) {
        return 0;
    }
    ez = (((u16)pt->z + 0x8000) >> 7 & 0x1FF) - m->z0;
    dz = ez;
    if ((u16)ez >= m->h) {
        return 0;
    }
    tbl = m->table;
    planes = m->planes;
    plain = m->plain;
    flagged = m->flagged;
    c = FLOOR_ADDRESS(u16, (u16)(((u32)dz * m->w + dx) * 2u) * 2 + m->cells);
    lists = m->lists;
    n = c[1];
    cursor = lists + c[0] + n * 2 - 2;
    while (n--) {
        v = *FLOOR_ADDRESS(u16, cursor);
        flag = v & 0x8000;
        if (flag == 0) {
            rec = plain + v * 18;
        } else {
            rec = flagged + (v & 0x7FFF) * 22;
        }
        if (func_801347A0(flag, FLOOR_ADDRESS(s16, rec), FLOOR_ADDRESS(FloorPlane, planes), FLOOR_ADDRESS(s32, tbl))) {
            D_8019F9C8 = *FLOOR_ADDRESS(u16, rec);
            return 1;
        }
        cursor -= 2;
    }
    return 0;
}
