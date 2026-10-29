/* func_801495C4 - owner re-derivation from retail assembly (clean input set).
 * Copies the u16 vector at p+2/6/A into a short vector, transforms it through
 * the sub-record's +0x34 block, converts it with func_8012EF70 and writes it back. */
#include "psx_types.h"

typedef struct { s16 vx, vy, vz, pad; } SVec;
typedef struct { u8 pad0[0x34]; u8 f34[1]; } Sub;
typedef struct { u8 pad0[0x20]; Sub *sub; } Obj;

extern void func_8012F14C(void *m, SVec *in, SVec *out);
extern void func_8012EF70(void *v, s32 a);

/* The inlined helper's parameters keep the third local's address out of
 * CSE, so it is recomputed at each call like retail. */
static inline void sub_transform(Obj *o, SVec *in, SVec *out) {
    func_8012F14C(o->sub->f34, in, out);
}

void func_801495C4(Obj *o, u8 *p) {
    SVec in;
    SVec out;
    SVec t;
    in.vx = *(u16 *)(p + 2);
    in.vy = *(u16 *)(p + 6);
    in.vz = *(u16 *)(p + 0xA);
    sub_transform(o, &in, &t);
    func_8012EF70(&t, (s32)&out);
    *(u16 *)(p + 2) = out.vx;
    *(u16 *)(p + 6) = out.vy;
    *(u16 *)(p + 0xA) = out.vz;
}
