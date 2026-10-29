/* Owner re-derivation from retail assembly (clean input set). */
/* Fills in one of three 16-byte table slots from the arguments, then notifies the slot handler. */
#include "psx_types.h"

typedef struct {
    s32 f0;
    s32 f4;
    s32 f8;
    u8 fC;
    u8 fD;
    u8 fE;
    u8 padF;
} Slot;

extern Slot D_800A5E88[];
extern void func_80028620(s32, Slot *);

void func_80028304(u16 idx, s32 a1, s32 a2, s32 a3, u8 b0, u8 b1, u8 b2) {
    if (idx < 3) {
        D_800A5E88[idx].f0 = a1;
        D_800A5E88[idx].f4 = a2;
        D_800A5E88[idx].f8 = a3;
        D_800A5E88[idx].fC = b0;
        D_800A5E88[idx].fD = b1;
        D_800A5E88[idx].fE = b2;
        func_80028620(idx, &D_800A5E88[idx]);
    }
}
