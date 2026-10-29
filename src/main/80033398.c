/* Owner re-derivation from retail assembly (clean input set). */
/* Finds active slots matching an id (and optional sub-id) and releases them and their voices. */
#include "psx_types.h"
typedef struct {
    u16 flags;
    u16 pad2;
    u16 id;
    u16 sub;
    u8 pad8[6];
    u8 used[8];
    u8 pad16[0x40 - 0x16];
    s32 f40;
    u8 pad44[0x4E - 0x44];
    u8 f4E;
    u8 pad4F[0x54 - 0x4F];
} Slot;
extern Slot D_800A46E8[];
extern void func_80030D80(Slot *, s32);
extern void func_80034650(Slot *, s32);

void func_80033398(u32 arg)
{
    s32 i, j;
    u16 lo;
    u32 hi;
    Slot *s;
    Slot *p;
    Slot *q;

    lo = arg;
    s = D_800A46E8;
    i = 0;
    hi = arg >> 16;
    for (; i < 8; i++, s++) {
        if ((s->flags & 0x3F) != 1) {
            continue;
        }
        if (s->id != lo) {
            continue;
        }
        if (hi != 0 && s->sub != hi) {
            continue;
        }
        p = &D_800A46E8[(u16)i];
        switch (p->flags & 0x3F) {
        case 1:
            for (j = 0; j < 8; j++) {
                if (p->used[j] != 0) {
                    p->used[j] = 0;
                    q = &D_800A46E8[8 + j];
                    q->f40 = 0;
                    if (q->f4E != 0) {
                        func_80030D80(q, 0);
                    }
                }
            }
            p->flags = 0;
            break;
        case 5:
            func_80034650(p, 0);
            break;
        }
    }
}
