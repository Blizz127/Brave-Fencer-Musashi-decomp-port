/* Owner re-derivation from retail assembly (clean input set). */
/* Dispatches to func_80150170 when the halfword triple at 6/A/E differs from the one at 0x88/0x8A/0x8C, else func_8014FE60. */
#include "psx_types.h"

extern void func_80150170(void *obj);
extern void func_8014FE60(void *obj);

void func_8014FDF4(u8 *obj) {
    if (*(s16 *)(obj + 6) != *(s16 *)(obj + 0x88) || *(s16 *)(obj + 0xA) != *(s16 *)(obj + 0x8A) ||
        *(s16 *)(obj + 0xE) != *(s16 *)(obj + 0x8C)) {
        func_80150170(obj);
    } else {
        func_8014FE60(obj);
    }
}
