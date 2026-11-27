#include "mine_sign.h"

extern int func_80178970(MineSign *sign);
extern void func_80178D18(MineSign *sign);

void func_8017D838(MineSign *sign)
{
    if (func_80178970(sign)) {
        func_80178D18(sign);
        sign->state = 1;
    }
}
