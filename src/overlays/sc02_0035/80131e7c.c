/* Owner re-derivation from retail assembly (clean input set). */
/* When flag bit 0x40000 of o->fB4 is set and func_8012E470(o) reports true, runs func_8012E4C8(o)
 * and func_80131E00(o, 1). Always finishes with func_80131CA8(o, 0x3A). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0xB4];
    u32 fB4;   /* 0xB4 */
} Obj;

extern s32 func_8012E470(Obj *o);
extern void func_8012E4C8(Obj *o);
extern void func_80131E00(Obj *o, s32 v);
extern void func_80131CA8(Obj *o, s32 v);

void func_80131E7C(Obj *o)
{
    if (o->fB4 & 0x40000) {
        if (func_8012E470(o) != 0) {
            func_8012E4C8(o);
            func_80131E00(o, 1);
        }
    }
    func_80131CA8(o, 0x3A);
}
