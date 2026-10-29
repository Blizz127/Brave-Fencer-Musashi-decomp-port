/* Owner re-derivation from retail assembly (clean input set). */
/* Distance test against a fixed point near D_80126B5E/62/66 (y lowered by 0x40): when the squared radius (r+0x20)^2 exceeds func_800132BC(p, point) it queues func_8012F568 with the object and D_8018480C and returns 1, else 0. */
#include "psx_types.h"

typedef struct {
    s16 x, y, z;
} Vec3s;

extern u16 D_80126B5E;
extern u16 D_80126B62;
extern u16 D_80126B66;
extern u8 D_8018480C[];
extern s32 func_800132BC(void *a, void *b);
extern void func_8012F568(s32 a, s32 b, s32 c, s32 d, void *e, void *f);

s32 func_8012D664(void *p, s32 r, s32 id)
{
    Vec3s v;

    v.x = D_80126B5E;
    v.y = D_80126B62 - 0x40;
    v.z = D_80126B66;
    if (func_800132BC(p, &v) < (r + 0x20) * (r + 0x20)) {
        func_8012F568(1, 1, 0, id, p, D_8018480C);
        return 1;
    }
    return 0;
}
