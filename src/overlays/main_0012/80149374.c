/* Owner re-derivation from retail assembly (clean input set). */
/* Converts a 16.16 vector to a short vector, transforms it through the sub-record's +0x34 block, and writes it back as 16.16. */
#include "psx_types.h"

typedef struct { s16 vx, vy, vz, pad; } SVec;
typedef struct { s32 vx, vy, vz, pad; } Vec;
typedef struct { u8 pad0[0x34]; u8 f34[1]; } Sub;
typedef struct { u8 pad0[0x20]; Sub *sub; } Obj;

extern void func_80015978(Vec *src, SVec *dst);
extern void func_8012F14C(void *m, SVec *in, SVec *out);
extern void func_80015954(SVec *src, Vec *dst);

/* The inlined helper's parameters keep the second local's address out of
 * CSE, so it is recomputed at each call like retail. */
static inline void sub_transform(Obj *o, SVec *in, SVec *out) {
    func_8012F14C(o->sub->f34, in, out);
}

void func_80149374(Obj *o, Vec *x) {
    SVec a;
    SVec b;
    func_80015978(x, &a);
    sub_transform(o, &a, &b);
    func_80015954(&b, x);
}
