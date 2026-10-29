/* Owner re-derivation from retail assembly (clean input set). */
/* If a music request is pending and loaded, starts its fade of the given type (clamped to 4) and sets the matching request timer. */
/* Verified 39/39 words with the pinned oracle, gcc-2.7.2-psx / ASPSX 2.56, -O2 -G0. */
#include "psx_types.h"

extern void func_8003819C(s16, s32, s32, s16);
extern s16 func_8003836C(s16);
extern u16 D_8006A9B0[];
extern s16 D_8006A9BC[];
extern s16 D_800A4E86;
extern u16 D_800A4E8A;
extern u16 D_800A4E8E;

void func_8002E700(u16 type) {
    if ((D_800A4E8E & 4) && func_8003836C(D_800A4E86) != 0) {
        if (type >= 5) {
            type = 4;
        }
        func_8003819C(D_800A4E86, 0, 0, D_8006A9BC[type]);
        D_800A4E8A = D_8006A9B0[type];
    }
}
