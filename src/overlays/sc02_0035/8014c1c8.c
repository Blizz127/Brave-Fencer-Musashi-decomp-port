/* Owner re-derivation from retail assembly (clean input set). */
/* Probes the segment from a's position to b's position + offset *off with func_80133784 (mode 1); * on a hit moves b to the returned end point. */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 x; /* 0x06 */
    u8 pad8[2];
    u16 y; /* 0x0A */
    u8 padC[2];
    u16 z; /* 0x0E */
} Obj8014C1C8;

extern s32 func_80133784(s32 a, s16 *p, s16 *q);

void func_8014C1C8(Obj8014C1C8 *a, Obj8014C1C8 *b, u16 *off)
{
    s16 from[3];
    s16 to[3];

    from[0] = a->x;
    from[1] = a->y;
    from[2] = a->z;
    to[0] = b->x + off[0];
    to[1] = b->y + off[1];
    to[2] = b->z + off[2];
    if (func_80133784(1, from, to) != 0) {
        b->x = to[0];
        b->y = to[1];
        b->z = to[2];
    }
}
