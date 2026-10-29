/* Owner re-derivation from retail assembly (clean input set). */
/* Chooses among three handlers depending on the flag word and on whether the halfword triples at 6/A/E and 0x88/0x8A/0x8C differ. */
#include "psx_types.h"

extern void func_8014D3E0(void *obj);
extern void func_8014D04C(void *obj);
extern void func_8014CCB4(void *obj);

void func_8014CC28(u8 *obj) {
    if (*(s32 *)(obj + 0x44) < 0) {
        if (*(s16 *)(obj + 0xA) != *(s16 *)(obj + 0x8A)) {
            func_8014D3E0(obj);
        } else if (*(s16 *)(obj + 6) != *(s16 *)(obj + 0x88) || *(s16 *)(obj + 0xE) != *(s16 *)(obj + 0x8C)) {
            func_8014D04C(obj);
        } else {
            func_8014CCB4(obj);
        }
    } else {
        func_8014CCB4(obj);
    }
}
