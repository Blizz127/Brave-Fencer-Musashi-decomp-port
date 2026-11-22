#include "mine_emitter_setup.h"

#if !defined(__mips__) || defined(__LP64__)
#define func_8012C1B8 bfm_mine_emitter_acquire_model
#define func_8001C2C4 bfm_mine_emitter_bind_model
#endif

void func_8017F050(MineEmitterSetup *actor)
{
    MineEmitterModel model = func_8012C1B8();
    actor->model = (u32)model;
    if (!model) {
        func_8012CAE4((u8 *)actor);
    } else {
        func_8001C2C4(model);
        actor->index = 0;
        actor->variant = 0;
        actor->timer = actor->initial_delay;
        func_8012AD50((MineEmitterStateActor *)actor);
    }
}
