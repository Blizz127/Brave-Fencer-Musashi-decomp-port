/* Owner re-derivation from retail assembly (clean input set). */
/* Draws the 64 on-screen text/sprite slots (types 1 and 2), then links a draw-mode (texture page) packet into the current ordering table. */
#include "psx_types.h"

typedef struct {
    s16 type;
    u8 pad[0xE];
} TextSlot;

typedef struct {
    u32 addr : 24;
    u32 len : 8;
} PrimTag;

typedef struct {
    PrimTag tag;
    u32 code;
} DrTpage;

typedef struct {
    u32 ot; /* guest PrimTag * */
    u8 pad[0x14 - 4];
} OtBuffer;

extern u8 D_800AF630[];
extern OtBuffer D_800A651C[];
extern u16 D_800B9A02;
extern u8 D_800B9A11;
extern u32 D_800A5E60; /* guest pointer: next free primitive */

extern void func_80015608(TextSlot *slot, PrimTag *ot);
extern void func_80015760(TextSlot *slot, PrimTag *ot);
extern u32 func_80058B40(s32 tp, s32 abr, s32 x, s32 y); /* GetTPage-shaped */

void func_80015498(void)
{
    u8 *work;
    PrimTag *ot;
    TextSlot *slot;
    DrTpage *p;

    work = D_800AF630;
    ot = (PrimTag *)D_800A651C[D_800B9A02].ot;
    if (D_800B9A11 == 1) {
        return;
    }
    for (slot = (TextSlot *)(work + 0x9DA8); slot < (TextSlot *)(work + 0xA1A8); slot++) {
        switch (slot->type) {
        case 0: /* empty slot; also gives the retail compare tree */
            break;
        case 1:
            func_80015608(slot, ot);
            break;
        case 2:
            func_80015760(slot, ot);
            break;
        }
    }
    p = (DrTpage *)D_800A5E60;
    p->tag.len = 1;
    p->code = (func_80058B40(0, 0, 0x140, 0x100) & 0x9FF) | 0xE1000000;
    p->tag.addr = ot->addr;
    ot->addr = (u32)p;
    D_800A5E60 += 8;
}
