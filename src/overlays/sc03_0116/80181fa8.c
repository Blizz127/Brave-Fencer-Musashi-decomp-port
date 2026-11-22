#include "actor_333.h"

#if defined(__mips__) && !defined(__LP64__)
#define ATTACHED(actor) ((actor)->model)
#define ACQUIRE(actor) ((actor)->model = func_8012C1B8())
#else
#define ATTACHED(actor) bfm_room_333_attached_model(actor)
static Actor333Model *acquire(Actor333 *actor)
{
    Actor333Model *model = func_8012C1B8();
    actor->model_address = model ? bfm_room_333_model_address(model) : 0;
    return model;
}
#define ACQUIRE(actor) acquire(actor)
#endif

s32 func_80181FA8(Actor333 *actor)
{
    Actor333Model *model;
    s32 next;

    if ((u8)func_80029178(0x11c) || !(model = ACQUIRE(actor))) {
        return func_8012CAE4((u8 *)actor);
    }

#if defined(__mips__) && !defined(__LP64__)
    func_8001C810(&model->prefix, D_80194EE0);
#else
    bfm_room_333_bind_model(model);
#endif
    ATTACHED(actor)->fields.flags |= 0x8000;
#if defined(__mips__) && !defined(__LP64__)
    actor->shape = (u32)D_801864E0 | 0x40000000;
#else
    actor->shape = bfm_room_333_shape_address | 0x40000000;
#endif
    actor->contact_flags = 0xc800;
    D_801864C8.value = 0;
    D_801864D4.value = 0;
#if defined(__mips__) && !defined(__LP64__)
    ATTACHED(actor)->fields.table = (u32)&D_801864C8;
#else
    ATTACHED(actor)->fields.table = bfm_room_333_table_address;
#endif
    actor->vector[0] = actor->vector[1] = actor->vector[2] = 0x80;
    actor->vector[3] = -2;
    ATTACHED(actor)->fields.options |= 0x80;
#if defined(__mips__) && !defined(__LP64__)
    ATTACHED(actor)->fields.vector = (u32)actor->vector;
#else
    ATTACHED(actor)->fields.vector = bfm_room_333_vector_address(actor);
#endif
    next = actor->phase + 1;
    actor->phase = (u16)next;
    return next;
}
