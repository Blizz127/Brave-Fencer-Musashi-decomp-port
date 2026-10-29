/* Owner re-derivation from retail assembly (clean input set). */
/* Transforms a 16.16 vector through the matrix chain at the object's link record and writes it back; the middle call goes through an inline helper so the second local's address is recomputed per call. */
#include "psx_types.h"

typedef struct { s16 x, y, z, w; } V;
typedef struct { u8 pad[0x20]; u32 link; } O;

extern void func_80015978(s32 a, V *v);
extern void func_8004914C(u32 p);
extern void func_800491AC(u32 p);
extern void func_8004945C(V *a, V *b, V *c);
extern void func_80015954(V *a, s32 b);

static inline void mid(V *a, V *b, V *c) {
    func_8004945C(a, b, c);
}

void func_8012F274(O *o, s32 a) {
    V v0;
    V v1;
    V v2;
    u32 s;
    func_80015978(a, &v0);
    s = o->link + 0x34;
    func_8004914C(s);
    func_800491AC(s);
    mid(&v0, &v1, &v2);
    func_80015954(&v1, a);
}
