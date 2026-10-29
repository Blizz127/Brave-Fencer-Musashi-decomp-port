/* Owner re-derivation from retail assembly (clean input set). */
/* If the s16 at 0xA exceeds 0x400, copies a 3-vector from o+4 into D_80126DB0, copies that 8-byte block to o+0x88, and sets the linked record's s16 at 0x12 from D_80126DB6. */
#include "psx_types.h"

typedef struct {
    u8 b[8];
} Blk80146fc4;

typedef struct {
    u8 pad0[0x12];
    u16 y; /* 0x12 */
} Rec80146fc4;

typedef struct {
    u8 pad0[0xA];
    s16 limit; /* 0xA */
    u8 pad1[0x20 - 0xC];
    u32 link; /* 0x20: guest pointer to Rec80146fc4 */
    u8 pad2[0x88 - 0x24];
    Blk80146fc4 blk; /* 0x88 */
} Obj80146fc4;

extern Blk80146fc4 D_80126DB0;
extern u16 D_80126DB6;

extern void func_80015954(void *src, void *dst);

void func_80146FC4(Obj80146fc4 *o)
{
    if (o->limit > 0x400) {
        func_80015954(&D_80126DB0, (u8 *)o + 4);
        o->blk = D_80126DB0;
        ((Rec80146fc4 *)o->link)->y = D_80126DB6;
    }
}
