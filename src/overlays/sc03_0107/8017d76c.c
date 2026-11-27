#include "mine_sign.h"

extern u8 D_801860E8[];
extern u8 D_801827D4[];
extern int func_8012C354(MineSign *sign, const void *resources);
extern void func_8012A828(MineSign *sign, const void *setup);
typedef struct MineSignInteraction MineSignInteraction;
extern void *func_8017D878(MineSignInteraction *interaction);
extern MineSignInteraction *func_801788B8(
    MineSign *sign, void *(*on_interaction)(MineSignInteraction *));

void func_8017D76C(MineSign *sign)
{
    if (func_8012C354(sign, D_801860E8)) {
        sign->field_c0 = 1;
        sign->state = 1;
        func_8012A828(sign, D_801827D4);
        func_801788B8(sign, func_8017D878);
    }
}
