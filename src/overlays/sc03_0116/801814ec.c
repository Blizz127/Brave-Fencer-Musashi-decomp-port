#include "actor_33a.h"

void func_801814EC(Actor33A *actor)
{
#if defined(__mips__) && !defined(__LP64__)
    u16 *model = func_8012C194();
    actor->placed.model_address = (u32)model;
    if (model) {
        func_8001CF00((Actor33AModel *)model);
#else
    u32 model = bfm_room_33a_acquire_model();
    actor->placed.model_address = model;
    if (model) {
        bfm_room_33a_initialize_model(model);
#endif
        actor->placed.x = 0;
        actor->placed.y = -256;
        actor->placed.z = 7872;
        func_8012AD50(&actor->state);
    } else {
        func_8012C098((char *)actor);
    }
}
