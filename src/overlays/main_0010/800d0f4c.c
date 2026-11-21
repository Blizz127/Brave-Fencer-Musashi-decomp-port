/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up id = func_800D0F8C(a0 & 0xFF); if found (nonzero), runs func_800D0FE0(id) and
 * returns id, else returns 0. */
#include "psx_types.h"

extern s32 func_800D0F8C(s32 id);
extern void func_800D0FE0(s32 h);

s32 func_800D0F4C(s32 a0)
{
    s32 id = func_800D0F8C(a0 & 0xFF);
    if (id == 0) {
        return 0;
    }
    func_800D0FE0(id);
    return id;
}
