/* Owner re-derivation from retail assembly (clean input set). */
/* Loads the D_800AF648 matrix (rotation + translation), transforms arg and
 * writes the first output word to *out (0 when the depth word is negative). */
#include "psx_types.h"

extern u8 D_800AF648;
extern void func_8004914C(void *p);
extern void func_800491AC(void *p);
extern void func_8004955C(void *a, s32 *x, s32 *y, s32 *z);

/* The inlined helper's parameter keeps &D_800AF648 out of CSE, so retail
 * recomputes the address for each call. */
static inline void set_matrix(void *m) {
    func_8004914C(m);
    func_800491AC(m);
}

/* Retail returns out in v0. */
s32 *func_8012F40C(s32 *out, void *arg) {
    s32 x, y, z;
    x = 0;
    set_matrix(&D_800AF648);
    func_8004955C(arg, &x, &y, &z);
    if (z < 0) {
        x = 0;
        *out = 0;
    } else {
        *out = x;
    }
    return out;
}
