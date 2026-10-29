/* Owner re-derivation from retail assembly (clean input set). */
/* Zeros 8 words of D_801B655C (stride 0x10, offsets 0..0x70), resets several u16 fields, then
 * func_80165124(o); o->f66=0; func_80146CA0(o). No caller found in any available region. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x2C];
    u16 f2C; /* 0x2C */
    u8 pad2E_[0x2E - 0x2E];
    u16 f2E; /* 0x2E */
    u8 pad30[0x60 - 0x30];
    u16 f60; /* 0x60 */
    u16 f62; /* 0x62 */
    u8 pad64[0x66 - 0x64];
    u16 f66; /* 0x66 */
} Obj;

extern u8 D_801B655C[];
extern void func_80165124(void *o);
extern void func_80146CA0(void *o);

void func_80164EE0(Obj *o)
{
    s32 i;

    for (i = 0x70; i >= 0; i -= 0x10) {
        *(u32 *)(D_801B655C + i) = 0;
    }

    o->f2E = 0x4000;
    o->f2C = 0x4000;
    o->f60 = 0;
    o->f62 = 0;
    func_80165124(o);
    o->f66 = 0;
    func_80146CA0(o);
}
