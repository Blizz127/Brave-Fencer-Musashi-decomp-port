/* Owner re-derivation from retail assembly (clean input set). */
/* Calls func_800484EC with a two-zero-word local record followed by the second argument. */
#include "psx_types.h"

extern void func_800484EC(void *p, void *rec, void *q);

void func_8012B178(u8 *obj, s32 arg) {
    s32 rec[3];
    rec[2] = arg;
    rec[1] = 0;
    rec[0] = 0;
    func_800484EC(*(u8 **)(obj + 0x20) + 0x34, rec, obj + 0x10);
}
