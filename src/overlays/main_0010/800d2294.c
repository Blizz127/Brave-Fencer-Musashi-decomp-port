/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void *func_801465C0(void *a0);
extern s32 func_8001CD9C(void *a0, void *a1);

void *func_800D2294(void *a0, void *a1) {
    void *x = func_801465C0(a0);
    if (x != 0) {
        func_8001CD9C(x, a1);
        return x;
    }
    return 0;
}
