/* func_8014EA4C [8014EA4C,8014ED28) - owner re-derivation from retail assembly (clean input set). */
/* Tests obj's probe point against every live collision entry of the 0x60-entry pool D_801202A0
 * (skipping the two entries obj already rides/holds); on a hit either pushes obj off the side
 * (flag bit 1 of D_801150D8 set, returns bit 2) or snaps it on top of the hit point (bit 1). */
#include "psx_types.h"

typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 w;
} V8;

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
} Ent8014EA4C;

extern Ent8014EA4C D_801202A0[];
extern u16 D_800B99DA;
extern s32 D_801150D8;
extern s16 D_801152AA;
extern s16 D_80126720;
extern s16 D_80126724;
/* shape is a guest address word (the callee uses it as a base pointer). */
extern s32 func_80135A4C(s32 shape, s32 kind, V8 *probe, V8 *out);

/* unused: the only caller (func_8014E98C) passes its pass counter in a3; never read here. */
s32 func_8014EA4C(u8 *obj, V8 *probe, V8 *out, s32 unused) {
    V8 hit;   /* unreferenced: retail frame keeps 16 bytes of locals before saved */
    V8 push;
    V8 saved; /* dead copy of *out (retail stores it at sp+0x20 and never reads it) */
    Ent8014EA4C *e;
    s32 result;
    s32 dx;
    s32 dz;

    result = 0;
    saved = *out;
    dx = 8;
    dz = 8;
    switch ((D_800B99DA & 0x60) >> 5) {
    case 0:
    case 1:
        dx = 7;
        break;
    case 2:
        dz = 7;
        break;
    }
    for (e = D_801202A0; e < &D_801202A0[96]; e++) {
        if (e->id == 0) continue;
        if (e->kind == 0) continue;
        if (e == *(Ent8014EA4C **)(obj + 0x178)) continue;
        if (e == *(Ent8014EA4C **)(obj + 0x174)) continue;
        if ((e->flags.w & 0x1800) == 0) continue;
        D_801150D8 = 0;
        if (func_80135A4C(e->shape, e->kind, probe, out) == 0) continue;
        if (D_801150D8 & 1) {
            if ((e->flags.h & 0x10) == 0) {
                if (D_80126720 - probe->x >= 0) {
                    *(u16 *)(obj + 6) = out->x - dx;
                } else {
                    *(u16 *)(obj + 6) = out->x + dx;
                }
                if (D_80126724 - probe->z >= 0) {
                    *(u16 *)(obj + 0xE) = out->z - dz;
                } else {
                    *(u16 *)(obj + 0xE) = out->z + dz;
                }
            }
            result |= 2;
        } else {
            *(u16 *)(obj + 6) = out->x;
            *(u16 *)(obj + 0xA) = out->y + 0x10;
            *(u16 *)(obj + 0xE) = out->z;
            result |= 1;
            if (e->kind < 0) {
                if ((s16)(D_801152AA < 0 ? -D_801152AA : D_801152AA) > 0x800) {
                    if (D_80126720 - out->x >= 0) {
                        *(u16 *)(obj + 6) = out->x - dx;
                    } else {
                        *(u16 *)(obj + 6) = out->x + dx;
                    }
                    if (D_80126724 - out->z >= 0) {
                        *(u16 *)(obj + 0xE) = out->z - dz;
                    } else {
                        *(u16 *)(obj + 0xE) = out->z + dz;
                    }
                }
            }
        }
    }
    return result;
}
