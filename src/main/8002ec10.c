/* Owner re-derivation from retail assembly (clean input set). */
/* Ends the current music request: either promotes it to the active track (if loaded and loopable) or unloads it, clearing the related request flags. */
#include "psx_types.h"

typedef struct {
    u16 flags;
    u8 pad2[0x10 - 2];
} TrackInfo;

extern s16 func_8003836C(s16);
extern void func_800383A4(s16);
extern void func_800384A8(s16);
extern void func_800385C0(s16);
extern TrackInfo D_80068B66[];
extern s16 D_800A4E86;
extern s16 D_800A4E8A;
extern u16 D_800A4E8C;
extern u16 D_800A4E8E;
extern s16 D_800A4E9A;
extern u16 D_800A4EA0;
extern u16 D_800A4EA2;
extern s16 D_800A4F20;
extern s16 D_800A4F22;

void func_8002EC10(void) {
    s16 *id;

    D_800A4F20 = 0;
    D_800A4F22 = 0;
    D_800A4E8A = 0;
    if (D_800A4E8E & 4) {
        if (func_8003836C(D_800A4E86) != 0 && (D_80068B66[D_800A4E8C].flags & 1)) {
            if (D_800A4EA2 & 4) {
                func_800385C0(D_800A4E9A);
                D_800A4EA2 &= 0xFEE8;
            }
            func_800383A4(D_800A4E86);
            D_800A4E8E &= ~2;
            D_800A4EA2 = D_800A4E8E;
            D_800A4E9A = D_800A4E86;
            D_800A4EA0 = D_800A4E8C;
            D_800A4E8E &= ~0x106;
            return;
        }
        id = &D_800A4E86;
        if (func_8003836C(*id) != 0) {
            func_800384A8(*id);
        }
        D_800A4E8E &= 0xFEE8;
        func_800385C0(*id);
    }
}
