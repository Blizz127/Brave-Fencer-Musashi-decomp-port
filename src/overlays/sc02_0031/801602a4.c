/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatch-table handler (main_0012.s D_80180E00 .word slot). If o->fB8 == 0x8000, runs the
 * per-kind handler D_8018730C[o->f0] then func_80154A74(o,0x11), func_80147078(o,0) and
 * func_80159B70(o); otherwise just func_80161208(o). */
#include "psx_types.h"

typedef struct {
    u16 f0;
    u8 pad2[0xB6];
    u16 fB8;
} Obj;

extern void (*D_8018730C[])(void);
extern void func_80154A74(void *o, s32 a);
extern void func_80147078(void *o, s32 a);
extern void func_80159B70(void *o);
extern void func_80161208(void *o);

#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_801602A4(Obj *o)
{
    if (o->fB8 == 0x8000) {
        D_8018730C[o->f0]();
        func_80154A74(o, 0x11);
        func_80147078(o, 0);
        func_80159B70(o);
    } else {
        func_80161208(o);
    }
    RETURN_NO_VALUE;
}
