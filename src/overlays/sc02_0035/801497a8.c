/* Owner re-derivation from retail assembly (clean input set). */
/* func_801497A8 [801497A8,80149864) - clean-room owner re-derivation (nmG). */
#include "psx_types.h"

typedef struct {
    u16 id;
    u8 pad02[0x42];
    u32 flags; /* 0x44 */
    u8 pad48[0x64];
    u16 state; /* 0xAC */
} Obj801497A8;

typedef struct {
    u8 pad0[0x49];
    u8 f49; /* 0x49: same byte as D_80078EC1 */
} Glob801497A8;

extern u8 D_80078E78[];
extern u8 D_80078EC1;
extern s32 func_8014CB7C(void);
extern void *func_8014C088(Obj801497A8 *unused, s32 id);
extern s32 func_80165658(Obj801497A8 *o, s32 a);

s32 func_801497A8(Obj801497A8 *o)
{
    Glob801497A8 *g;

    g = ((Glob801497A8 *)&D_80078E78);
    if (o->id == 0x1A) {
        return 0;
    }
    if (func_8014CB7C() != 0) {
        return 0;
    }
    if (D_80078EC1 == 0xF && func_8014C088(o, 0xA) != 0) {
        return 0;
    }
    if (o->flags & 0x404) {
        goto fail;
    }
    if (func_80165658(o, g->f49) & 0x80) {
        goto fail;
    }
    if (o->state & 0x20) {
        return 1;
    }
fail:
    return 0;
}
