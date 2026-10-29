/* Owner re-derivation from retail assembly (clean input set). */
/* Projects v through the D_800AF648 matrix and, if the flag is not negative and
 * the screen point is inside the 0x18F x 0x153 window around the centre,
 * forwards (a, b) to func_8002D4C8. */
#include "psx_types.h"

extern u8 D_800AF648;
extern void func_8004914C(void *p);
extern void func_800491AC(void *p);
extern void func_8004955C(void *v, void *sxy, void *p, void *flag);
extern void func_8002D4C8(s32 a, s32 b);

/* The inlined helper's parameter keeps &D_800AF648 out of CSE, so retail
 * recomputes the address for each call. */
static inline void set_matrix(void *m) {
    func_8004914C(m);
    func_800491AC(m);
}

/* Callers do not read v0. */
void func_8012E5CC(void *v, u16 a, u16 b)
{
    s32 r[3]; /* [0] = screen x/y halves, [1] = p, [2] = flag */
    set_matrix(&D_800AF648);
    func_8004955C(v, &r[0], &r[1], &r[2]);
    if (r[2] >= 0 && (u16)(((u16 *)r)[0] + 0xC7) < 0x18F && (u16)(((u16 *)r)[1] + 0xA9) < 0x153) {
        func_8002D4C8(a, b);
    }
}
