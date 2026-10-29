/* func_800173DC: builds a Gouraud triangle (POLY_G3) from a 3-vertex record, projected through a matrix or taken as
 * screen xy, and inserts it into the ordering table. */
#include "psx_types.h"

typedef struct {
    u32 xy; /* vx | vy << 16 */
    s16 z;
    s16 pad;
} TriVert;

typedef struct {
    TriVert v[3];
    u32 rgb[3];
    u32 flags;
} TriDesc;

extern void *func_80010A08(s32 size);
extern void func_80058D98(u32 *p);
extern void func_80017E8C(void *mtx);
extern s32 func_8004958C(TriVert *v0, TriVert *v1, TriVert *v2, u32 *sxy0, u32 *sxy1, u32 *sxy2, s32 *p,
                         s32 *flag);
extern void func_80018094(void *prim, u32 depth, u32 flags);

void func_800173DC(TriDesc *q, void *mtx) {
    u32 *p; /* POLY_G3: tag, rgb0, xy0, rgb1, xy1, rgb2, xy2 */
    s32 otz;
    s32 res[2];

    p = func_80010A08(0x1C);
    p[1] = q->rgb[0];
    p[3] = q->rgb[1];
    p[5] = q->rgb[2];
    func_80058D98(p);
    if (mtx != 0) {
        func_80017E8C(mtx);
        otz = func_8004958C(&q->v[0], &q->v[1], &q->v[2], &p[2], &p[4], &p[6], &res[0], &res[1]);
    } else {
        p[2] = q->v[0].xy;
        p[4] = q->v[1].xy;
        p[6] = q->v[2].xy;
        otz = q->v[0].z;
        res[1] = 0;
    }
    if ((res[1] & ~0x1000) == 0) {
        func_80018094(p, otz, q->flags);
    }
}
