/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the first live, flagged object in D_801202A0 within range 0x30 of obj, else 0. */
#include "psx_types.h"

typedef struct Obj8014A2E4 {
    /* 0x000 */ u16 active;
    /* 0x002 */ u8 pad002[0x5A];
    /* 0x05C */ u16 flags;
    /* 0x05E */ u8 pad05E[0x4C];
    /* 0x0AA */ s16 hp;
    /* 0x0AC */ u8 pad0AC[0x60];
} Obj8014A2E4; /* size 0x10C */

extern Obj8014A2E4 D_801202A0[];

extern s32 func_8014C278(void *a, void *b, u16 range);

Obj8014A2E4 *func_8014A2E4(void *obj)
{
    u32 i;
    Obj8014A2E4 *p;

    i = 0;
    p = D_801202A0;
loop:
    if (p->active != 0 && (p->flags & 0x8000) && p->hp != 0) {
        if (func_8014C278(obj, p, 0x30)) {
            return p;
        }
    }
    i++;
    p++;
    if (i < 0x60) {
        goto loop;
    }
    return 0;
}
