/* Owner re-derivation from retail assembly (clean input set). */
/* If o->fB8 bit 0x8000 is set, dispatches to D_8018730C[o->f0] (discarding its result), then
 * func_80147078(o,0); func_80159B70(o); otherwise runs func_80161208(o). No caller found in any
 * available region. */
#include "psx_types.h"

typedef struct {
    u16 f0; /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8; /* 0xB8 */
} Obj;

typedef void (*Handler)(Obj *);

extern u32 D_8018730C[];
extern void func_80147078(void *o, u16 a);
extern void func_80159B70(void *o);
extern void func_80161208(void *o);

void func_8015F9A4(Obj *o)
{
    if (o->fB8 & 0x8000) {
        ((Handler)D_8018730C[o->f0])(o);
        func_80147078(o, 0);
        func_80159B70(o);
    } else {
        func_80161208(o);
    }
}
