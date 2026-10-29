/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_80147948 with a clamp limit of 0x1F0000. */
#include "psx_types.h"

extern void func_80147948(void *obj, s32 delta, s32 limit);

void func_80147928(void *obj, s32 delta) {
    func_80147948(obj, delta, 0x1F0000);
}
