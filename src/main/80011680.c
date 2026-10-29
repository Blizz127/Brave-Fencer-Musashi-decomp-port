/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80011818(s32);                     /* extern */
void func_8002D4C8(s32, s32);            /* extern */
extern u8 D_800AF630[];

void func_80011680(void) {
    register u8 *base = D_800AF630;
    (*(s8 *)((u8 *)(base) + (0xA434))) = 0;
    func_8002D4C8(0x29, 0);
    func_80011818(5);
}
