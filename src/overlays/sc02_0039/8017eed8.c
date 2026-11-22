#include "mine_child_model.h"

void func_8017EED8(MineChildModelActor *actor)
{
    MineChildModelRecord *record;
#if !defined(__mips__) || defined(__LP64__)
    if (!actor || actor->model_index < 0 || actor->model_index >= 3)
        return;
#endif
    record = D_8019B618[actor->model_index];
    if (record) {
        func_8012B2CC((char *)actor);
        if (func_8017EF5C(actor, record)) {
            u16 feedback = record->feedback_a;
            D_80126B96 = feedback;
            feedback = record->feedback_b;
            D_80126B98 = feedback;
        }
    }
}
