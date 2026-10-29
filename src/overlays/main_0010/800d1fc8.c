/* Owner re-derivation from retail assembly (clean input set). */
/* Offsets the object's three position halfwords by the camera-relative vector (camera - pos, reset to a straight-down vector when any component leaves +-0x3FFF) transformed in place by func_80047E98 and shifted right by sh. */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} SV800D1FC8;

typedef struct {
    u8 pad00[6];
    u16 x;
    u16 pad08;
    u16 y;
    u16 pad0C;
    u16 z;
} Obj800D1FC8;

typedef struct {
    u8 pad00[0x5C];
    s32 x;
    s32 y;
    s32 z;
} Cam800D1FC8;

extern s32 func_80047E98(SV800D1FC8 *in, SV800D1FC8 *out);
extern u32 D_801151D4;

void func_800D1FC8(Obj800D1FC8 *o, u16 sh)
{
    SV800D1FC8 t;
    Cam800D1FC8 *c;

    c = (Cam800D1FC8 *)D_801151D4;
    t.x = c->x - o->x;
    t.y = c->y - o->y;
    t.z = c->z - o->z;
    if (!(t.x >= -0x3FFF && t.x <= 0x3FFF && t.y >= -0x3FFF && t.y <= 0x3FFF && t.z >= -0x3FFF && t.z <= 0x3FFF)) {
        t.z = 0;
        t.x = 0;
        t.y = -0xFFF;
    }
    func_80047E98(&t, &t);
    o->x += t.x >> sh;
    o->y += t.y >> sh;
    o->z += t.z >> sh;
}
