#include "mine_sign.h"

extern u32 D_80186134[];
extern u32 D_8018611C[];
extern void func_80178CBC(MineSign *sign, const u32 *script);

void func_8017D7D8(MineSign *sign)
{
    if (sign->variant != 0) {
        func_80178CBC(sign, D_80186134);
    } else {
        func_80178CBC(sign, D_8018611C);
    }
    sign->state = 3;
}
