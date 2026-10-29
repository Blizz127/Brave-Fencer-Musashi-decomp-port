/* Owner re-derivation from retail assembly (clean input set). */
/* Unconditionally sets o->fC1 to 7; if the 0x80 flag bit of o->fB4 is set, runs
 * func_80131170(o, D_8018484C, 0xB); always finishes with func_80131CA8(o, 0x16). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0xB4];
    u32 fB4;     /* 0xB4 */
    u8 pad0B8[0xC1 - 0xB8];
    u8 fC1;      /* 0xC1 */
} Obj;

extern void func_80131170(Obj *o, void *a1, s32 a2);
extern void func_80131CA8(Obj *o, s32 a1);
extern u8 D_8018484C[];

void func_8012F8C8(Obj *o)
{
    o->fC1 = 7;
    if (o->fB4 & 0x80) {
        func_80131170(o, D_8018484C, 0xB);
    }
    func_80131CA8(o, 0x16);
}
