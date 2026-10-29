/* func_8014DCE0 - clean-room owner re-derivation (nmG). */
/* Copies the 16-byte record behind o->rec onto the stack, then probes up to 3 consecutive 8-byte targets
 * (starting at `to`) with func_80135888; returns 1 on the first hit, else 0. */
#include "psx_types.h"

typedef struct {
    u32 w[4];
} Rec8014DCE0;

typedef struct {
    u8 pad0[0x20];
    u32 shape; /* 0x20: guest address */
    u8 pad24[0x34];
    u32 rec;   /* 0x58: guest address of a 16-byte record */
} Obj8014DCE0;

extern s32 func_80135888(u32 obj, u32 flags, u32 from, u32 to);

s32 func_8014DCE0(Obj8014DCE0 *o, u32 from, u32 to)
{
    Rec8014DCE0 v;
    s32 i;
    u32 p;

    v = *(Rec8014DCE0 *)(o->rec & 0xFFFFFFF);
    for (i = 0, p = to; i < 3; i++, p += 8) {
        if (func_80135888(o->shape, (u32)&v, from, p) != 0) {
            return 1;
        }
    }
    return 0;
}
