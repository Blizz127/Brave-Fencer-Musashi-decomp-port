/* func_8012F49C - owner re-derivation, clean room (nmO).
 * Retail `move v0,s2; sw v1,0(v0)` stores through the return register: the
 * result pointer r is a pseudo local to the join block, so local-alloc ties it
 * to v0 (it dies into the return copy) and the w reload takes v1. The extra
 * `r = out;` at entry is dead (flow deletes it) but makes r born before the
 * join's CSE block, so CSE keeps r as the canonical register instead of
 * folding it back into out. Returns out (s32 *, v0 = a0 in retail);
 * callers read v0: none-direct. */
#include "psx_types.h"

extern void func_8004914C(int p);
extern void func_800491AC(int p);
extern void func_8004945C(int a, int b, int *out);
extern void func_8004955C(s32, s32, s32, s32);
extern u8 D_800AF648;

static inline void pre(int d) {
    func_8004914C(d);
}

static inline void post(int d) {
    func_800491AC(d);
}

s32 *func_8012F49C(s32 *out, u8 *obj, s32 a) {
    s32 p[2];
    s32 w[2];
    s32 xyz[4];
    s32 m;
    s32 *pp;
    s32 *r;

    r = out;
    m = *(s32 *)(obj + 0x20) + 0x34;
    func_8004914C(m);
    func_800491AC(m);
    pp = p;
    func_8004945C(a, (int)pp, w);
    xyz[0] = 0;
    pre((int)&D_800AF648);
    post((int)&D_800AF648);
    func_8004955C((s32)pp, (s32)&xyz[0], (s32)&xyz[1], (s32)&xyz[2]);
    if (xyz[2] < 0) {
        xyz[0] = 0;
        w[0] = 0;
    } else {
        w[0] = xyz[0];
    }
    r = out;
    *r = w[0];
    return r;
}
