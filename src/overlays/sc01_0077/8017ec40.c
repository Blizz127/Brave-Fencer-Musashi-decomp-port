#include "forest_settle_effect.h"

typedef void (*ForestSettleStep)(ForestSettleEffect *);
extern ForestSettleStep D_8018A7B8[];

void func_8017EC40(ForestSettleEffect *effect)
{
    D_8018A7B8[effect->state](effect);
}
