/* Owner re-derivation from retail assembly (clean input set). */
/* Offsets pos by the camera-relative vector (camera - pos, reset to a straight-down vector when any component leaves +-0x3FFF) transformed by func_80047E98 and shifted right by sh. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} SV800D20C0;

typedef struct {
    u16 x;
    u16 y;
    u16 z;
} UV800D20C0;

typedef struct {
    u8 pad00[0x5C];
    s32 x;
    s32 y;
    s32 z;
} Cam800D20C0;

extern s32 func_80047E98(SV800D20C0 *in, SV800D20C0 *out);
extern u32 D_801151D4;

void func_800D20C0(UV800D20C0 *pos, SV800D20C0 *out, u16 sh)
{
    SV800D20C0 t;
    Cam800D20C0 *c;

    c = (Cam800D20C0 *)D_801151D4;
    t.x = c->x - pos->x;
    t.y = c->y - pos->y;
    t.z = c->z - pos->z;
    if (!(t.x >= -0x3FFF && t.x <= 0x3FFF && t.y >= -0x3FFF && t.y <= 0x3FFF && t.z >= -0x3FFF && t.z <= 0x3FFF)) {
        t.z = 0;
        t.x = 0;
        t.y = -0xFFF;
    }
    func_80047E98(&t, out);
    pos->x += out->x >> sh;
    pos->y += out->y >> sh;
    pos->z += out->z >> sh;
}
