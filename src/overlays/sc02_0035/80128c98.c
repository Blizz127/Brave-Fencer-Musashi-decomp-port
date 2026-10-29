/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u32 w[2];
} Elem8;

extern s16 D_80184780[];
extern Elem8 D_800AE830[];
extern s32 D_801BC3F0;
extern s32 func_8001ABBC(s32, s32, void *, s32, s32);

s32 func_80128C98(s32 idx) {
    s16 v = D_80184780[idx];
    if (v < 0) {
        return 1;
    }
    return func_8001ABBC(0, 0, &D_800AE830[v], D_801BC3F0, 0);
}
