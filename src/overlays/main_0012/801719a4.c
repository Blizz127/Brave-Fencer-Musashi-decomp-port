/* Owner re-derivation from retail assembly (clean input set). */
/* Copies n words from the object's table (starting at its +0x214 cursor) into D_8011F738 and advances the cursor by n. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x1FC];
    u32 *tbl;
    u8 pad200[0x14];
    u8 base;
} Obj;

extern u32 D_8011F738;

void func_801719A4(Obj *o, u32 n) {
    u32 i;
    u32 off;
    for (i = 0; i < n; i++) {
        off = i * 4;
        *(u32 *)((u8 *)((u32 *)&D_8011F738) + off) = o->tbl[o->base + i];
    }
    o->base += i;
}
