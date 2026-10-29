/* Owner re-derivation from retail assembly (clean input set). */
/* Three successive post-increment lookups into D_8018758C (signed bytes) by o->f234, each summed
 * with a field of the sub-record at o->f178->f78 and stored into o->f178's output halfwords, then
 * forwards the +4 vector to func_80149374. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0xC];
    u16 fC;
    u16 fE;
    u16 f10;
} R;

typedef struct {
    u8 pad0[0x6];
    u16 f6;
    u8 pad8[0x2];
    u16 fA;
    u8 padC[0x2];
    u16 fE;
    u8 pad10[0x68];
    R *f78;
} S;

typedef struct {
    u8 pad0[0x178];
    S *f178;
    u8 pad17C[0xB8];
    s32 f234;
} Obj;

extern u8 D_8018758C[];
extern void func_80149374(Obj *o, void *v);

void func_8015DCB8(Obj *o)
{
    S *s = o->f178;
    s32 c;

    c = o->f234;
    o->f234 = c + 1;
    s->f6 = (s8)D_8018758C[c] + s->f78->fC;

    c = o->f234;
    o->f234 = c + 1;
    s->fA = (s8)D_8018758C[c] + s->f78->fE;

    c = o->f234;
    o->f234 = c + 1;
    s->fE = (s8)D_8018758C[c] + s->f78->f10;

    func_80149374(o, (u8 *)s + 4);
}
