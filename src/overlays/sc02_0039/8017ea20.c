#include "mine_child_setup.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#define ACQUIRE_MODEL func_8012C1B8
#define MODEL(word) ((MineChildSetupModel *)(word))
#define PARENT(word) ((MineChildSetup *)(word))
#define BIND_MODEL(model) func_8001C214((MineChildListModel *)(model), D_8019E298)
#define TRANSFORM func_800484EC
#define SHAPE_ADDRESS ((u32)D_8019B624)
#define CHILD_MODEL MODEL(actor->model)
#define PARENT_MODEL MODEL(PARENT(actor->parent)->model)
#else
#define ACQUIRE_MODEL bfm_mine_emitter_acquire_model
#define MODEL(word) bfm_mine_child_setup_model(word)
#define PARENT(word) bfm_mine_child_setup_parent(word)
#define BIND_MODEL(model) bfm_mine_child_setup_bind(model, D_8019E298)
#define TRANSFORM bfm_mine_child_setup_transform
#define SHAPE_ADDRESS bfm_mine_child_setup_shape_address
#define CHILD_MODEL model
#define PARENT_MODEL parent_model
#endif

void func_8017EA20(MineActorStage *stage)
{
    MineChildSetup *actor = (MineChildSetup *)stage;
    MineEmitterModel acquired;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    MineChildSetupModel *model;
    MineChildSetupModel *parent_model;
    MineChildSetup *parent;
#endif
    s32 sample;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (!actor)
        return;
#endif
    acquired = ACQUIRE_MODEL();
    actor->model = (u32)acquired;
    if (!acquired) {
        func_8012CAE4((u8 *)actor);
        return;
    }
    BIND_MODEL(acquired);
    actor->variant = actor->subtype & 15;
    if (actor->variant) {
        u32 scale;
        s32 variation = func_8005C49C() & 255;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        model = MODEL(actor->model);
        if (!model || (u16)actor->variant >= 3)
            return;
#endif
        scale = D_8019B5B4[actor->variant].scale;
        CHILD_MODEL->flags |= 0x10;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        model = MODEL(actor->model);
        if (!model)
            return;
#endif
        CHILD_MODEL->scale_x = CHILD_MODEL->scale_y = CHILD_MODEL->scale_z = (s16)(scale + variation);
    }
    actor->motion = 0x18000;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if ((u16)actor->variant >= 3 || !D_8019B5E6[actor->variant].divisor)
        return;
#endif
    /* The original writes the rate-derived step before reading model scale. */
    actor->step = 0x1000 / D_8019B5E6[actor->variant].divisor;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    model = MODEL(actor->model);
    if (!model)
        return;
#endif
    if (CHILD_MODEL->scale_x >= 0x1000)
        actor->step = 0x40;
    else
        actor->step = (0x1400 - CHILD_MODEL->scale_x) >> 4;
    if (actor->variant != 2) {
        s32 yaw = func_8005C49C();
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        parent = PARENT(actor->parent);
        if (!parent)
            return;
        parent_model = MODEL(parent->model);
        model = MODEL(actor->model);
        if (!parent_model || !model)
            return;
#endif
        CHILD_MODEL->yaw = PARENT_MODEL->yaw + (yaw & 255) - 0x80;
    } else {
        s32 choice = func_8005C49C() & 1;
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        parent = PARENT(actor->parent);
        if (!parent)
            return;
        parent_model = MODEL(parent->model);
        model = MODEL(actor->model);
        if (!parent_model || !model)
            return;
#endif
        CHILD_MODEL->yaw = PARENT_MODEL->yaw + choice * 448 - 224;
    }
    func_8012B2CC((char *)actor);
#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    model = MODEL(actor->model);
    if (!model || (u16)actor->variant >= 3)
        return;
#endif
    TRANSFORM(CHILD_MODEL->transform, D_8019B5B4[actor->variant].offset, actor->position);
    sample = func_8005C49C();
    actor->shape = SHAPE_ADDRESS | 0x20000000u;
    actor->shape |= 0x40000000u;
    actor->contact_flags = 0xc000;
    actor->state_byte = 0;
    actor->z -= sample & 31;
    func_8012AD50((MineEmitterStateActor *)actor);
}
