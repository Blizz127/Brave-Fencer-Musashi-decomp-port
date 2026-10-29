/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u16 h0, h2, h4, h6, h8, hA;
} P;

typedef struct {
    s16 x, y, z, w;
} V;

typedef struct {
    u8 pad[0x20];
    u32 link;
} O;

extern void func_8012F038(u32 a, V *in, V *out);

void func_801494CC(O *o, P *p, P *d) {
    V r;
    V in;
    in.x = p->h2;
    in.y = p->h6;
    in.z = p->hA;
    func_8012F038(o->link + 0x34, &in, &r);
    d->h2 = r.x;
    d->h6 = r.y;
    d->hA = r.z;
}