/* func_80016A7C: builds a Gouraud line (LINE_G2) from a 2-vertex record, projected through a matrix or taken as
 * screen xy, and inserts it into the ordering table. */
#include "psx_types.h"

typedef struct {
    u32 xy; /* vx | vy << 16 */
    s16 z;
    s16 pad;
} LineVert;

typedef struct {
    LineVert v[2];
    u32 rgb[2];
    u32 flags;
} LineDesc;

extern void *func_80010A08(s32 size);
extern void func_80058EB0(u32 *p);
extern void func_80017E8C(void *mtx);
extern s32 func_8004958C(LineVert *v0, LineVert *v1, LineVert *v2, u32 *sxy0, u32 *sxy1, u32 *sxy2, s32 *p,
                         s32 *flag);
extern void func_80018094(void *prim, u32 depth, u32 flags);

void func_80016A7C(LineDesc *q, void *mtx) {
    u32 *p; /* LINE_G2: tag, rgb0, xy0, rgb1, xy1 */
    s32 otz;
    s32 res[2];

    p = func_80010A08(0x14);
    p[1] = q->rgb[0];
    p[3] = q->rgb[1];
    func_80058EB0(p);
    if (mtx != 0) {
        func_80017E8C(mtx);
        otz = func_8004958C(&q->v[0], &q->v[1], &q->v[1], &p[2], &p[4], &p[4], &res[0], &res[1]);
    } else {
        p[2] = q->v[0].xy;
        p[4] = q->v[1].xy;
        otz = q->v[0].z;
        res[1] = 0;
    }
    if ((res[1] & ~0x1000) == 0) {
        func_80018094(p, otz, q->flags);
    }
}
