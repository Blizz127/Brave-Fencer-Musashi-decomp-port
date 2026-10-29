/* Owner re-derivation from retail assembly (clean input set). */
/* Initialises an entry: optionally resets it from four globals, then fills in its position, size and mode fields. */
#include "psx_types.h"

typedef struct Entry {
    s32 f0;
    s32 f4;
    s32 f8;
    s32 fC;
    s32 f10;
    s32 f14;
    s16 f18;
    s16 f1A;
    s16 f1C;
    s16 f1E;
    s16 f20;
    s16 f22;
    s32 f24;
    s32 f28;
    s16 f2C;
    s16 f2E;
    s16 f30;
    s16 f32;
    s32 f34;
    s32 f38;
} Entry;

typedef struct Src {
    s32 f0;
    s32 f4;
} Src;

extern s32 D_800D9470;
extern s32 D_800D9474;
extern s32 D_800D9478;
extern s32 D_800D947C;
extern s32 D_800D9480;

void func_800D2394(Entry *arg0, s16 arg1, s16 arg2, s16 arg3, s32 arg4, Src *arg5) {
    s16 mode;

    if (D_800D9480 == 1) {
        arg0->f8 = 0;
        arg0->f14 = 0;
        arg0->f28 = 0;
        arg0->f34 = 0;
        D_800D9480 = 0;
        arg0->f0 = D_800D9470;
        arg0->f4 = D_800D9474;
        arg0->fC = D_800D9478;
        arg0->f10 = D_800D947C;
    }
    arg0->f18 = arg1;
    arg0->f1A = arg2;
    arg0->f20 = arg3;
    arg0->f22 = arg4;
    mode = 0x10;
    if (arg5->f4 != 0) {
        mode = 0x18;
    }
    arg0->f30 = mode;
    arg0->f38 = arg5->f4;
    if (arg0->f28 == 0) {
        arg0->f2C = arg1;
        arg0->f2E = arg2;
        return;
    }
    arg0->f2C = arg3;
    arg0->f2E = arg4;
}
