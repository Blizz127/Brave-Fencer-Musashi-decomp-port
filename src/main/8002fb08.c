/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up the record for an id, releases its resources, marks it used and sets a global flag; returns 1, or 0 if the precondition fails. */
#include "psx_types.h"

extern u8 D_80064D49[][12];
extern u8 D_80064D4D[][12];
extern s32 D_800A463C[][6];
extern s16 D_800A4640[][12];
extern s16 D_800A4642[][12];
extern u8 D_800A4650[][24];
extern s32 D_800A46C8;
extern s16 D_800A46CC;
extern s16 D_800C5328[][2];
extern u8 D_8006AEF4;
extern void func_800301A4(void);
extern s32 func_80037CD8(void (*cb)(void));
extern void func_80031A98(void);
extern void func_8002EC10(void);
extern void func_800415A8(s32 a);

s32 func_8002FB08(s32 arg0) {
    s32 sub;
    u8 kind;

    if (func_80037CD8(func_800301A4) == 0) {
        return 0;
    }
    kind = D_80064D49[arg0][0];
    D_800A46C8 = D_800A463C[kind][0];
    if (D_800A4650[kind][0] == 0) {
        D_800C5328[D_800A4640[kind][0]][0] = -1;
    }
    func_80031A98();
    sub = D_80064D4D[arg0][0];
    if (sub != 0) {
        if (D_800A4642[sub][0] >= 0) {
            func_8002EC10();
            func_800415A8(D_800A4642[sub][0]);
            D_800A4642[sub][0] = -1;
            D_800A4640[sub][0] = -1;
        }
        D_800A4650[sub][0] = 1;
    }
    D_800A46CC = 0;
    D_8006AEF4 |= 2;
    return 1;
}
