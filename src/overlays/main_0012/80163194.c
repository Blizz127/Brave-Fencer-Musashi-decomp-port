/* Owner re-derivation from retail assembly (clean input set). */
/* Runs func_80162F60/80162F80/80162FA0 with a, b, c, transforms o+0x24 by the
 * stack argument d into the scratch words D_80127090..98 and applies them with
 * func_80146D30 (sibling of func_8016325C). */
#include "psx_types.h"

typedef struct { s32 x, y, z; } V3;
typedef struct { u8 p0[0x24]; u8 f24[1]; } O;

extern void func_80162F60(O *o, s32 a);
extern void func_80162F80(O *o, s32 a);
extern void func_80162FA0(O *o, s32 a);
extern void func_800484EC(void *p, void *rec, void *q);
extern void func_80146D30(O *o);
extern s32 D_80127090;
extern s32 D_80127094;
extern s32 D_80127098;

/* Callers do not read v0. func_80162F60 takes (o, a) like in func_8016325C. */
void func_80163194(O *o, s32 a, s32 b, s32 c, void *d) {
    s32 pad[4];
    V3 v;
    func_80162F60(o, a);
    func_80162F80(o, b);
    func_80162FA0(o, c);
    func_800484EC(d, o->f24, &v);
    D_80127090 = v.x;
    D_80127094 = v.y;
    D_80127098 = v.z;
    func_80146D30(o);
}
