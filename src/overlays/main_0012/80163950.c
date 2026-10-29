/* func_80163950 [80163950,80163A50) - owner re-derivation from retail assembly (clean input set).
 * Finds the first record of the 0x60-record pool D_801202A0 (0x10C bytes each) for which
 * func_80135A4C passes, then calls func_80015954 and returns 1. */
#include "psx_types.h"

typedef struct {
    u16 used;
    u8 p2[0x1E];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
    u16 f5C;
    u8 p5E[0xAE];
} Rec; /* 0x10C */

/* callers read v0: none-direct (no jal caller); returns 1/0.
 * func_80135A4C: owner prototype (s32, s32, V8 *, V8 *); pointers declared void * here. */
extern void func_80015978(void *src, void *dst);
extern void func_80015954(void *src, void *dst);
extern s32 func_80135A4C(u32 shape, u32 kind, void *probe, void *out);
extern u8 D_801202A0[];

s32 func_80163950(u8 *o) {
    u16 v[4];
    u16 w[4];
    Rec *r;

    func_80015978(o + 4, v);
    func_80015978(o + 4, w);
    v[1] -= 0x20;
    w[1] += 8;
    for (r = ((Rec *)&D_801202A0); r < ((Rec *)&D_801202A0) + 0x60; r++) {
        if (r->used != 0 && (r->f5C & 0x400) != 0 && r->f58 != 0) {
            if (func_80135A4C(r->f20, r->f58, v, w) != 0) {
                func_80015954(w, o + 4);
                return 1;
            }
        }
    }
    return 0;
}
