/* Owner re-derivation from retail assembly (clean input set). */
/* If func_80178970(o) is nonzero: runs func_80178D18(o), looks up D_80188C14[o->0x70 (s16)]
 * and forwards that byte to func_80029124(byte, 1), then func_8012C218(o). */
#include "psx_types.h"

extern s32 func_80178970(u8 *o);
extern s32 func_80178D18(u8 *o);
extern u8 D_80188C14[];
extern void func_80029124(s32 a, s32 b);
extern void func_8012C218(char *o);

void func_8017F1F4(u8 *o)
{
    if (func_80178970(o) == 0) {
        return;
    }
    func_80178D18(o);
    func_80029124(D_80188C14[*(s16 *)(o + 0x70)], 1);
    func_8012C218((char *)o);
}
