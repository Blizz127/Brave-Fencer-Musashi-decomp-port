/* Owner re-derivation from retail assembly (clean input set). */
/* Returns 1 if the object is idle (+0x178 == 0, no 0x404 flags, +0xAC bit 0x80 set) and func_80149F2C accepts
 * it against any entry of the 0x60-entry pool D_801202A0 (clearing +0x190 first), else 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x44];
    u32 flags;   /* 0x44 */
    u8 pad48[0x64];
    u16 state;   /* 0xAC */
    u8 padAE[0xCA];
    u32 busy;    /* 0x178 */
    u8 pad17C[0x14];
    u32 scan;    /* 0x190 */
} Obj80149DD8;

typedef struct {
    u8 b[0x10C];
} Ent80149DD8;

extern u8 D_801202A0[];
extern s32 func_80149F2C(Obj80149DD8 *o, Ent80149DD8 *e);

s32 func_80149DD8(Obj80149DD8 *o)
{
    Ent80149DD8 *e;

    if (o->busy != 0) {
        return 0;
    }
    if (o->flags & 0x404) {
        return 0;
    }
    if ((o->state & 0x80) == 0) {
        return 0;
    }
    o->scan = 0;
    for (e = ((Ent80149DD8 *)&D_801202A0); e < &((Ent80149DD8 *)&D_801202A0)[0x60]; e++) {
        if (func_80149F2C(o, e) != 0) {
            return 1;
        }
    }
    return 0;
}
