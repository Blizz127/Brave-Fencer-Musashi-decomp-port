/* Owner re-derivation from retail assembly (clean input set). */
/* If the word at +0x1F8 is set, optionally calls func_80148634(obj) (bit 0x1000000), then calls the handler address in the low masked bits with the object. */
#include "psx_types.h"

typedef struct {
    u8 pad000[0x1F8];
    /* 0x1F8 */ u32 handler1F8;
} Obj8016F14C;

extern void func_80148634(Obj8016F14C *obj);

void func_8016F14C(Obj8016F14C *obj)
{
    u32 flags = obj->handler1F8;
    u32 fn = flags & 0x80FFFFFF;

    if (flags != 0) {
        if (flags & 0x1000000) {
            func_80148634(obj);
        }
        ((void (*)(Obj8016F14C *))fn)(obj);
    }
}
