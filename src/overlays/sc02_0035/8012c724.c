/* Owner re-derivation from retail assembly (clean input set). */
/* Sets bit 15 of the flags at +0xA and chains to func_8012C890. */
#include "psx_types.h"

extern void func_8012C890(void *o, s32 a, s32 b);

typedef struct {
    u8 pad0[0xA];
    u16 flags;
} Obj8012C724;

void func_8012C724(Obj8012C724 *o, s32 a) {
    o->flags |= 0x8000;
    func_8012C890(o, a, 0);
}
