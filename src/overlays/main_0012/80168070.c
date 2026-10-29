/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a rotation matrix aimed from the midpoint of two points toward a fixed target, using the segment direction and the heading to the target. */
#include "psx_types.h"

extern s32 D_801269A4;
extern s32 D_801269A8;
extern s32 D_801269AC;
extern void func_80149350(void *obj, void *a, void *b);
extern void func_800D20C0(void *a, void *b, s32 n);
extern void func_80017E68(void *a, void *b);
extern void func_80048FBC(void *a, void *b, void *c);
extern void func_800D23D0(void *a);
extern void func_80049A1C(void *a, void *b);
extern void func_8004D504(void *a, void *b, void *c);
extern s32 func_8004CFEC(s32 a, s32 b);

void func_80168070(u8 *obj, s16 *p1, s16 *p2, void *out) {
    s16 mid[4];
    s16 dir[4];
    s32 d[3];
    mid[0] = (p1[0] + p2[0]) >> 1;
    mid[1] = (p1[1] + p2[1]) >> 1;
    mid[2] = (p1[2] + p2[2]) >> 1;
    func_80149350(obj, mid, mid);
    func_800D20C0(mid, dir, 8);
    func_80017E68(mid, out);
    dir[0] = p2[0] - p1[0];
    dir[1] = p2[1] - p1[1];
    dir[2] = p2[2] - p1[2];
    func_80048FBC(*(u8 **)(obj + 0x20) + 0x34, dir, dir);
    func_800D23D0(dir);
    func_80049A1C(dir, out);
    d[0] = D_801269A4 - mid[0];
    d[1] = D_801269A8 - mid[1];
    d[2] = D_801269AC - mid[2];
    func_8004D504(out, d, d);
    dir[2] = -func_8004CFEC(d[0], d[1]);
    func_80049A1C(dir, out);
}
