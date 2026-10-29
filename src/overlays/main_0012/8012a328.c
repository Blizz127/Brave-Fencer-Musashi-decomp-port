/* Owner re-derivation from retail assembly (clean input set). */
/* Eases D_80126942 toward a chosen target (1/4 step) and copies the other two components from D_80126B58. */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u8 pad000[0x154];
    /* 0x154 */ s16 vec[3];
} Glob8012A328;

extern u8 D_801152C0;
extern s16 D_801152C2;
extern s16 D_80127080;
extern s16 D_80126CAE;
extern s16 D_80126940;
extern s16 D_80126942;
extern s16 D_80126944;
extern Glob8012A328 D_80126B58;

extern s16 func_80012F74(s16 cur, s16 target, s16 shift, s16 mode);

void func_8012A328(void)
{
    Glob8012A328 *g;
    s16 *p;
    s16 *q;

    g = &D_80126B58;
    if (D_801152C0 != 0) {
        if (D_801152C2 < ((D_80127080 - D_80126CAE) < 0 ? -(D_80127080 - D_80126CAE) : (D_80127080 - D_80126CAE))) {
            p = &D_80126942;
            *p = func_80012F74(*p, g->vec[1], 4, 1);
        } else {
            p = &D_80126942;
            *p = func_80012F74(*p, D_80127080, 4, 1);
        }
    } else {
        q = &D_80126942;
        *q = func_80012F74(*q, D_80126CAE, 4, 1);
    }
    D_80126940 = g->vec[0];
    D_80126944 = g->vec[2];
}
