/* func_8014F2E0 [8014F2E0,8014F3E8) - owner re-derivation from retail assembly (clean input set).
 * Scans the 0x60-record pool D_801202A0 (0x10C bytes each) for a record that passes func_80135A4C. */
#include "psx_types.h"

typedef struct {
    u16 used;
    u8 p2[0x1E];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
    u32 f5C;
    u8 p60[0xAC];
} Rec; /* 0x10C */

typedef struct {
    u8 p0[0x174];
    u32 f174;
    u32 f178;
} O;

/* callers read v0: yes (func_8014F24C returns it); returns 1 if any record hit, else 0.
 * func_80135A4C: owner prototype (s32, s32, V8 *, V8 *); pointers declared void * here. */
extern s32 func_80135A4C(u32 shape, u32 kind, void *probe, void *out);
extern u8 D_801202A0[];
extern s32 D_801150D8;

s32 func_8014F2E0(O *o, void *a, void *b) {
    s32 res;
    Rec *r;

    res = 0;
    for (r = ((Rec *)&D_801202A0); r < ((Rec *)&D_801202A0) + 0x60; r++) {
        if (r->used != 0 && r->f58 != 0 && (u32)r != o->f178 && (u32)r != o->f174 &&
            (r->f5C & 0x1800) != 0) {
            D_801150D8 = 0;
            if (func_80135A4C(r->f20, r->f58, a, b) != 0) {
                res = 1;
            }
        }
    }
    return res;
}
