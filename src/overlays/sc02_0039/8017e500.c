/* Enable or hide a mine collision actor and test its active box segments. */
#include "mine_collision_actor_active.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u16 kind, state;
    u8 before_x[2];
    s16 x;
    u8 before_model[24];
    void *model;
    u8 before_flags[0x38];
    u16 flags;
    u8 before_parent[6];
    void *parent;
    u8 before_mode[12];
    u8 mode;
    u8 before_variant[137];
    s16 variant;
    u8 before_last_mode[2];
    u8 last_mode;
} MineActor;

typedef struct {
    u32 reserved, flags;
} MineModel;

extern s16 D_80126CAC[];
extern u16 D_80126B96, D_80126B98;
extern SceneSurfaceBox D_8019B57C;
/* The caller consumes the transform service's memory effects. */
extern void func_8017E024(Obj8014A2E4 *entity);

void func_8017E500(Obj8014A2E4 *entity)
{
    MineActor *actor = (MineActor *)entity;
    s16 distance;
    if ((u16)actor->variant < 2) {
        distance = actor->x - D_80126CAC[0];
        if (distance < 0)
            distance = -distance;
        if (distance > 2048) {
            ((MineModel *)actor->model)->flags |= 0x80000000;
        } else {
            func_8017E25C(entity);
            ((MineModel *)actor->model)->flags &= 0x7fffffff;
            func_8017E024(entity);
        }
    } else {
        if ((s32)((MineModel *)((MineActor *)actor->parent)->model)->flags < 0) {
            actor->flags = 0;
        } else {
            actor->flags |= 0x4400;
            func_8017E024(entity);
            if (actor->variant == 3 && func_8017E2F4(entity, &D_8019B57C)) {
                D_80126B96 = 0x4007;
                D_80126B98 = 0x20;
            }
            actor->last_mode = actor->mode;
        }
    }
}
#else
#include "mine_actor_fields.h"

static uint8_t *actor_model(const Obj8014A2E4 *actor, const MineActiveServices *services)
{
    if (!services->model)
        return 0;
    return services->model(services->context, mine_actor_load32((const uint8_t *)actor + 0x20));
}

int mine_actor_active(Obj8014A2E4 *actor, const MineActiveServices *services)
{
    if (!actor || !services)
        return 0;
    uint8_t *data = (uint8_t *)actor;
    if (mine_actor_load16(data + 0xfe) < 2) {
        uint16_t delta = (uint16_t)(mine_actor_load16(data + 6) - services->player_x);
        int distance = mine_actor_signed16(delta);
        if (distance < 0)
            distance = mine_actor_signed16((uint16_t)-distance);
        if (distance > 2048) {
            uint8_t *model = actor_model(actor, services);
            if (!model)
                return 0;
            mine_actor_store32(model + 4, mine_actor_load32(model + 4) | UINT32_C(0x80000000));
            return 1;
        }
        if (!mine_actor_motion(actor, services))
            return 0;
        uint8_t *model = actor_model(actor, services);
        if (!model)
            return 0;
        mine_actor_store32(model + 4, mine_actor_load32(model + 4) & UINT32_C(0x7fffffff));
        return services->transform && services->transform(services->context, actor);
    }

    if (!services->parent)
        return 0;
    const Obj8014A2E4 *parent = services->parent(services->context, mine_actor_load32(data + 0x64));
    uint8_t *parent_model = parent ? actor_model(parent, services) : 0;
    if (!parent_model)
        return 0;
    if (mine_actor_load32(parent_model + 4) & UINT32_C(0x80000000)) {
        mine_actor_store16(data + 0x5c, 0);
        return 1;
    }
    mine_actor_store16(data + 0x5c, mine_actor_load16(data + 0x5c) | 0x4400);
    if (!services->transform || !services->transform(services->context, actor))
        return 0;
    if (mine_actor_load16(data + 0xfe) == 3) {
        int hit;
        if (!mine_actor_sweep(actor, services, &hit))
            return 0;
        if (hit) {
            if (!services->event_code)
                return 0;
            *services->event_code = 0x4007;
            if (!services->event_time)
                return 0;
            *services->event_time = 0x20;
        }
    }
    data[0x102] = data[0x74];
    return 1;
}
#endif
