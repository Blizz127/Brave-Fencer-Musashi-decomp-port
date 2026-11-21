#include "psx_types.h"

extern void func_8016F398(void);
extern void func_8017D900(void);

void func_8017DEC0(u8 *player)
{
    func_8016F398();
    ++player[0x214];
    func_8017D900();
}
