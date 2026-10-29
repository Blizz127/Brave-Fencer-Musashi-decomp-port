/* func_80132F40 [80132F40,80133060) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u16 x, y, z; } V3;
typedef struct { u16 x, y, z, w; } V4;

typedef struct {
    u8 p0[0x20];
    u32 f20;
    u8 p24[0x34];
    u32 f58;
} O;

/* callers read v0: none-direct (no jal caller); void. f20/f58 hold guest addresses.
 * The s16 min local m gets two orphan reload slots (16 B of frame) instead of a pad array. */
extern s32 func_80135888(u32 obj, u32 flags, void *from, void *to);
extern void func_8012F038(u32 a, void *in, void *out);
extern void func_8012F14C(u32 a, void *in, void *out);
extern u16 D_80126B5E;
extern u16 D_80126B62;
extern u16 D_80126B66;
extern u8 D_80126BE0[];

void func_80132F40(O *o) {
    V4 a;
    V4 b;
    V4 *pb;
    s16 *q;
    s16 m;

    pb = &b;
    a.x = D_80126B5E;
    a.y = D_80126B62;
    a.z = D_80126B66;
    b = a;
    if (func_80135888(o->f20, o->f58, D_80126BE0, pb) != 0) {
        q = (s16 *)((o->f58 & 0xFFFFFFF) | 0x80000000);
        if (q[5] < q[4]) {
            m = q[5];
        } else {
            m = q[4];
        }
        func_8012F038(o->f20 + 0x34, &a, pb);
        b.y = m;
        func_8012F14C(o->f20 + 0x34, pb, &a);
        D_80126B5E = a.x;
        D_80126B62 = a.y;
        D_80126B66 = a.z;
    }
}
