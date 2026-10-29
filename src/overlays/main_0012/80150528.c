/* func_80150528 [80150528,80150600) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 w;
} V8;

/* pool entry (same layout as in func_8014EA4C); shape is a guest address word. */
typedef struct {
    u16 id;
    u8 pad02[0x1E];
    s32 shape;
    u8 pad24[0x34];
    s32 kind;
    union {
        u32 w;
        u16 h;
    } flags;
    u8 pad60[0xAC];
} Ent;

/* callers read v0: none-direct (no jal caller); returns 1 and records the entry in o+0x1A4 on a hit, else 0. */
extern u8 D_801202A0[];
extern s32 func_80135A4C(s32 shape, s32 kind, V8 *probe, V8 *out);

s32 func_80150528(u8 *o, V8 *probe, V8 *out) {
    Ent *e;

    for (e = ((Ent *)&D_801202A0); e < &((Ent *)&D_801202A0)[96]; e++) {
        if (e->id == 0) continue;
        if ((e->flags.h & 0x80) == 0) continue;
        if (e->kind == 0) continue;
        if (func_80135A4C(e->shape, e->kind, probe, out) == 0) continue;
        *(u32 *)(o + 0x1A4) = (u32)e;
        return 1;
    }
    return 0;
}
