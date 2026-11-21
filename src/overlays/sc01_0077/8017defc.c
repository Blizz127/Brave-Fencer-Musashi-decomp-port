#include "psx_types.h"

extern u16 D_80126980;
extern void func_8017D98C(void);
extern void func_80171B44(u8 *player);

void func_8017DEFC(u8 *player)
{
    D_80126980 |= 0x8000;
    if (D_80126980 & 0x8000) {
        func_8017D98C();
        func_80171B44(player);
        ++player[0x214];
    }
}
