/* Owner re-derivation from retail assembly (clean input set). */
/* Allocates a record with func_801465C0, initialises it from the table pointer a with func_8001CB6C, stores the byte n at +0x27 and returns it; returns 0 when none is free. Twin of func_800D21C4. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x27];
    u8 f27;
} Rec800D222C;

extern u16 *func_801465C0(void);
extern void func_8001CB6C(u16 *h, s32 a, s32 b, s32 c);

Rec800D222C *func_800D222C(void *o, u8 *a, s32 n)
{
    Rec800D222C *r;
    Rec800D222C *res;

    r = (Rec800D222C *)func_801465C0();
    if (r == 0) {
        res = 0;
    } else {
        func_8001CB6C((u16 *)r, (s32)a, 0, 0);
        r->f27 = n;
        res = r;
    }
    return res;
}
