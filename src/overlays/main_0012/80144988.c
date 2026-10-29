/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct {
    /* 0x00 */ u8 pad00[2];
    /* 0x02 */ u16 cnt;
    /* 0x04 */ u8 pad04[6];
    /* 0x0A */ u16 y;
    /* 0x0C */ u8 pad0C[4];
    /* 0x10 */ u32 w10;
    /* 0x14 */ u32 w14;
    /* 0x18 */ u32 w18;
    /* 0x1C */ u8 pad1C[0x48];
    /* 0x64 */ u32 src; /* guest pointer: Obj80144988 */
} Obj80144988;

void func_80144988(Obj80144988 *o)
{
    o->y = o->y - 0x40;
    o->w10 = ((Obj80144988 *)o->src)->w10;
    o->w14 = ((Obj80144988 *)o->src)->w14;
    o->w18 = ((Obj80144988 *)o->src)->w18;
    o->cnt = o->cnt + 1;
}
