/* func_8001C0C8: clear the 256 x 0x38 and 192 x 0x84 record tables and one halfword. */
#include "psx_types.h"

extern u8 D_800AF630[];
void func_80016714(void *p, s32 n);

void func_8001C0C8(void) {
    u8 *state = D_800AF630;
    u8 *p;
    s32 i;

    p = state + 0x65A8;
    for (i = 0; i < 256; i++) {
        func_80016714(p, 0x38);
        p += 0x38;
    }
    p = state + 0x2A8;
    for (i = 0; i < 192; i++) {
        func_80016714(p, 0x84);
        p += 0x84;
    }
    *(s16 *)(state + 0x19C) = 0;
}
