/* func_80017294: builds a flat quad (POLY_F4) from a 4-vertex record, projected through a matrix or taken as
 * screen xy, and inserts it into the ordering table. */
#include "psx_types.h"

typedef struct {
    u32 xy; /* vx | vy << 16 */
    s16 z;
    s16 pad;
} QuadVert;

typedef struct {
    QuadVert v[4];
    u32 rgb;
    u32 flags;
} QuadDesc;

extern void *func_80010A08(s32 size);
extern void func_80058DC0(u32 *p);
extern void func_80017E8C(void *mtx);
extern s32 func_8004961C(QuadVert *v0, QuadVert *v1, QuadVert *v2, QuadVert *v3, u32 *sxy0, u32 *sxy1,
                         u32 *sxy2, u32 *sxy3, s32 *p, s32 *flag);
extern void func_80018094(void *prim, u32 depth, u32 flags);

void func_80017294(QuadDesc *q, void *mtx) {
    u32 *p; /* POLY_F4: tag, rgb, xy0..xy3 */
    s32 otz;
    s32 res[2];

    p = func_80010A08(0x18);
    p[1] = q->rgb;
    func_80058DC0(p);
    if (mtx != 0) {
        func_80017E8C(mtx);
        otz = func_8004961C(&q->v[0], &q->v[1], &q->v[2], &q->v[3], &p[2], &p[3], &p[4], &p[5], &res[0],
                            &res[1]);
    } else {
        p[2] = q->v[0].xy;
        p[3] = q->v[1].xy;
        p[4] = q->v[2].xy;
        p[5] = q->v[3].xy;
        otz = q->v[0].z;
        res[1] = 0;
    }
    if ((res[1] & ~0x1000) == 0) {
        func_80018094(p, otz, q->flags);
    }
}
