/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct Obj Obj;
typedef void (*Fn)(Obj *);

struct Obj {
    u16 f0;       /* 0x0 */
    u8 pad2[0xB8 - 0x2];
    u16 fB8;      /* 0xB8 */
};

extern s32 func_80161208(Obj *o);
extern Fn D_8018730C[];
extern void func_80147078(Obj *o, u16 a1);
extern void func_80159BE4(Obj *o);

void func_8015E698(Obj *o) {
    if (func_80161208(o) == 0) {
        if ((o->fB8 & 0x8000) != 0) {
            D_8018730C[o->f0](o);
            func_80147078(o, 0);
            func_80159BE4(o);
        }
    }
}
