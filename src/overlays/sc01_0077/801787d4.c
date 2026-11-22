/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80178840 for each of the 0x60 active entries (stride 0x10C) of table D_801202A0. */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u16 id;
    u8 pad002[0xBE];
    /* 0x0C0 */ u8 active;
    u8 pad0C1[0x4B];
} Ent801787D4;

extern Ent801787D4 D_801202A0[];
extern void func_80178840(Ent801787D4 *e);

void func_801787D4(void)
{
    s32 i;
    Ent801787D4 *e = D_801202A0;

    for (i = 0; i < 0x60; i++, e++) {
        if (e->id != 0 && e->active != 0) {
            func_80178840(e);
        }
    }
}
