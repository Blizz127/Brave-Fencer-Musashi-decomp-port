/* func_8014D610 [8014D610,8014D720) - owner re-derivation from retail assembly (clean input set).
 * Scans the 0x60-record pool D_801202A0 (0x10C bytes each) for a record that passes func_80135A4C,
 * claims it (o->f174, record flag) and copies the probe result into o. */
#include "psx_types.h"

typedef struct {
    u16 used;
    u8 p2[8];
    s16 fA;
    u8 pC[0x14];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
    u16 f5C;
    u8 p5E[0x16];
    u8 f74;
    u8 f75;
    u8 p76[0x96];
} Rec; /* 0x10C */

typedef struct {
    u8 p0[6];
    u16 f6;
    u8 p8[2];
    s16 fA;
    u8 pC[2];
    u16 fE;
    u8 p10[0x164];
    u32 f174;
    u8 p178[0xF8];
} O;

/* callers read v0: none-direct (the caller jumps on); returns 1 if a record was claimed, else 0.
 * func_80135A4C: owner prototype (s32, s32, V8 *, V8 *); pointers declared void * here. */
extern s32 func_80135A4C(u32 shape, u32 kind, void *probe, void *out);
extern u8 func_8014C918(O *o, s32 a);
extern u8 D_801202A0[];

s32 func_8014D610(O *o, void *a, u16 *b) {
    Rec *r;

    for (r = ((Rec *)&D_801202A0); r < ((Rec *)&D_801202A0) + 0x60; r++) {
        if (r->used != 0 && (r->f5C & 0x400) != 0 && r->f58 != 0 && r->fA >= o->fA &&
            func_80135A4C(r->f20, r->f58, a, b) != 0) {
            o->f174 = (u32)r;
            r->f74 = 1;
            o->f6 = b[0];
            o->fA = b[1];
            o->fE = b[2];
            *(u16 *)((u8 *)o + 0x16E) = func_8014C918(o, r->f75);
            return 1;
        }
    }
    return 0;
}
