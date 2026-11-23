#include "steamwood_valve.h"

void func_8017DB84(struct ValvePlayer *player)
{
    unsigned char progress = func_800291B4(206);
    unsigned int index = player->selector;
    if (progress < D_80183BC5[index] && progress >= D_80183BC4[index]) {
        func_800291A0(206, D_80183BC5[index]);
        func_80171A04(player);
    } else {
        func_800291A0(206, 1);
        func_8017EE30();
        func_80171A10(player);
    }
}
