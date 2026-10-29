/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a trail-style effect: stashes func_8012C1B8()'s table slot at +0x20 (detaches via
 * func_8012CAE4 and stops when there is none); otherwise runs func_8001C214(slot, 0), sets
 * +0x5C = 0x80, bumps the spawn counter at +0x2, and installs the D_801849F8/D_801849FC
 * (20-byte row) record for the packed index at +0x70 (low 12 bits) into +0x58/+0xDC, then
 * packs the index's top nibble (raw +0x70 >> 12) into +0xFC as a <<9 value. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x2];
    u16 h2;        /* 0x2 */
    u8 pad04[0x20 - 0x4];
    u32 f20;       /* 0x20: guest pointer, func_8012C1B8() slot */
    u8 pad24[0x58 - 0x24];
    u32 f58;       /* 0x58: guest pointer into the D_801849FC row view */
    u16 h5C;       /* 0x5C */
    u8 pad5E[0x70 - 0x5E];
    u16 h70;       /* 0x70: packed index (low 12 bits) / flags (top nibble) */
    u8 pad72[0xDC - 0x72];
    s32 wDC;       /* 0xDC */
    u8 padE0[0xFC - 0xE0];
    u16 hFC;       /* 0xFC */
} Obj;

typedef struct {
    s32 w0;
    u8 pad[16];
} Rec; /* 20 bytes */

extern u32 func_8012C1B8(void);
extern void func_8012CAE4(Obj *o);
extern void func_8001C214(u32 slot, s32 a);
extern Rec D_801849F8[];
extern u8 D_801849FC[];

void func_80132018(Obj *o)
{
    u32 slot;
    u16 idx;

    slot = func_8012C1B8();
    o->f20 = slot;
    if (slot == 0) {
        func_8012CAE4(o);
        return;
    }

    func_8001C214(slot, 0);
    o->h5C = 0x80;
    o->h2 = o->h2 + 1;

    idx = o->h70 & 0xFFF;
    o->f58 = (u32)(D_801849FC + idx * 20);

    idx = o->h70 & 0xFFF;
    o->wDC = D_801849F8[idx].w0;

    o->hFC = (o->h70 >> 0xC) << 9;
}
