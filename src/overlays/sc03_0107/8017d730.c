#include "mine_sign.h"

extern void (*D_8018614C[])(MineSign *sign);

void func_8017D730(MineSign *sign)
{
    D_8018614C[sign->state](sign);
}
