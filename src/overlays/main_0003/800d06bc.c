/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_800118AC();                                  /* extern */
void func_800D12D0();                                  /* extern */
extern s32 D_800EC890;
extern u8 D_800AF630[];

void func_800D06BC(void) {
    register u8 *unused = D_800AF630;
    func_800D12D0();
    D_800EC890 += 4;
    if (D_800EC890 >= 0x80) {
        D_800EC890 = 0x80;
        func_800118AC();
    }
}
