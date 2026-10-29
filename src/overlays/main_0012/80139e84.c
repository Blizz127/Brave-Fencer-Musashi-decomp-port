/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80139A8C(u8 *o);
extern void func_80139B18(u8 *o);

typedef struct {
    u8 p0[4];
    s16 st;
    u8 p6[2];
    u32 flags;
    u8 pc[2];
    u16 cnt;
} O;

void func_80139E84(O *o) {
    u32 f;
    if (o->cnt == 0) {
        o->st = 2;
    } else {
        func_80139A8C((u8 *)o);
        if (o->flags & 0x80020) {
            o->st = 2;
            f = o->flags;
            o->cnt = 0;
            if (!(f & 0x20000)) {
                o->flags = f & ~0x20;
            }
        }
    }
    func_80139B18((u8 *)o);
}
