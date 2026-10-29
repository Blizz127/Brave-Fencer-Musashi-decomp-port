/* Owner re-derivation from retail assembly (clean input set). */
/* Fills the 28-byte slot D_801B79E8[i]: enabled word 1, the three halfwords copied from the vector at a1 and the word 0x100 at +0xC (frame-pointer style, compiled -O0). */
#include "psx_types.h"

typedef struct {
    s32 on;
    u16 x;
    u16 y;
    u16 z;
    u16 pad;
    s32 f;
    u8 rest[0xC];
} Slot;

typedef struct {
    u16 x, y, z;
} V3;

extern Slot D_801B79E8[];

void func_8013B598(s32 i, V3 *v)
{
    D_801B79E8[i].on = 1;
    D_801B79E8[i].x = v->x;
    D_801B79E8[i].y = v->y;
    D_801B79E8[i].z = v->z;
    D_801B79E8[i].f = 0x100;
}
