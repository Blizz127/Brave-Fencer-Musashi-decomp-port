/* Owner re-derivation from retail assembly (clean input set). */
/* Builds a short vector from D_80126B5E, D_80126B62 and D_80126B66 and passes it with the incoming a0 to func_8012BE98; returns its result. */
#include "psx_types.h"

typedef struct {
    s16 x, y, z;
} Vec3s;

extern u16 D_80126B5E;
extern u16 D_80126B62;
extern u16 D_80126B66;
extern s32 func_8012BE98(void *p, Vec3s *v);

s32 func_8012BE54(void *p)
{
    Vec3s v;

    v.x = D_80126B5E;
    v.y = D_80126B62;
    v.z = D_80126B66;
    return func_8012BE98(p, &v);
}
