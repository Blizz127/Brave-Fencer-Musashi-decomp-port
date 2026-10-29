/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a Gouraud quad (POLY_G4) from a 4-vertex record, projected through a matrix or taken as screen xy, and inserts it into the ordering table. */
#include "psx_types.h"

typedef struct {
    u32 xy; /* vx | vy << 16 */
    s16 z;
    s16 pad;
} QuadVert;

typedef struct {
    QuadVert v[4];
    u32 rgb[4];
    u32 flags;
} QuadDesc;

extern void *func_80010A08(s32 size);
extern void func_80058DE8(u32 *p);
extern void func_80017E8C(void *mtx);
extern s32 func_8004961C(QuadVert *v0, QuadVert *v1, QuadVert *v2, QuadVert *v3,
                         u32 *sxy0, u32 *sxy1, u32 *sxy2, u32 *sxy3, s32 *p, s32 *flag);
extern void func_80017F14(void *prim, u32 depth, u32 flags);
extern void func_80018094(void *prim, u32 depth, u32 flags);

void func_80017778(QuadDesc *q, void *mtx, s32 mode)
{
    u32 *p; /* POLY_G4: tag, then rgb/xy word pairs */
    s32 otz;
    s32 res[2]; /* RotTransPers4 interpolation value, flag */

    p = func_80010A08(0x24);
    p[1] = q->rgb[0];
    p[3] = q->rgb[1];
    p[5] = q->rgb[2];
    p[7] = q->rgb[3];
    func_80058DE8(p);
    if (mtx != 0) {
        func_80017E8C(mtx);
        otz = func_8004961C(&q->v[0], &q->v[1], &q->v[2], &q->v[3],
                            &p[2], &p[4], &p[6], &p[8], &res[0], &res[1]);
    } else {
        p[2] = q->v[0].xy;
        p[4] = q->v[1].xy;
        p[6] = q->v[2].xy;
        p[8] = q->v[3].xy;
        otz = q->v[0].z;
        res[1] = 0;
    }
    if ((res[1] & ~0x1000) == 0) {
        if (mode != 0) {
            func_80017F14(p, otz, q->flags);
        } else {
            func_80018094(p, otz, q->flags);
        }
    }
}
