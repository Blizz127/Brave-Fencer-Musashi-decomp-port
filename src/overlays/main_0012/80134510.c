/* Owner re-derivation from retail assembly (clean input set). */
/* Vertical probe at a point: writes the segment (x, y - 4, z) .. (x, y + 0x2FC, z) to the two probe
 * points D_8017E910/D_8017E914, runs the cell-map hit test func_801345F8 on D_80184120 and, on a hit,
 * moves the point to the hit height minus 2 and returns a slope class (0x2000/0x4000/0x8000) taken
 * from the hit record's w field, which is then replaced by the hit id D_80184130. Returns 0 on a miss. */
#include "psx_types.h"

/* Point passed in (4 shorts; w is cleared). */
typedef struct {
    u16 x;
    u16 y;
    u16 z;
    u16 w;
} Pt;

/* Probe/hit record; D_8017E910/14/18 hold guest addresses of these. */
typedef struct {
    u16 x;
    u16 y;
    u16 z;
    s16 w;
} Hit;

extern u32 D_8017E910;
extern u32 D_8017E914;
extern u32 D_8017E918;
extern u32 D_80184120;
extern u16 D_80184130;
extern int func_801345F8(u32 map);

int func_80134510(Pt *o)
{
    Hit *b = (Hit *)D_8017E914;
    Hit *a = (Hit *)D_8017E910;
    int r = 0;
    u16 x;
    u16 y;
    u16 z;

    o->w = 0;
    x = o->x;
    a->x = x;
    b->x = x;
    y = o->y;
    a->y = y - 4;
    b->y = y + 0x2FC;
    z = o->z;
    a->z = z;
    b->z = z;
    if (func_801345F8(D_80184120)) {
        s16 w;

        o->y = ((Hit *)D_8017E918)->y - 2;
        w = ((Hit *)D_8017E910)->w;
        if (w >= -0xBCB) {
            if (w < -0x578) {
                r = 0x4000;
            } else {
                r = 0x8000;
            }
        } else {
            r = 0x2000;
        }
        ((Hit *)D_8017E910)->w = D_80184130;
    }
    return r;
}
