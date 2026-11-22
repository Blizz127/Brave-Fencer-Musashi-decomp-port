/* Dampen the model's rotation and copy the high half of the resulting angle. */
#include "mine_collision_actor_active.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u8 before_model[32];
    u8 *model;
    u8 before_position[184];
    s32 position;
    s32 velocity;
} MineMotion;

void func_8017E25C(Obj8014A2E4 *entity)
{
    MineMotion *actor = (MineMotion *)entity;
    s16 position = *(s16 *)((u8 *)&actor->position + 2);
    if (position < -2)
        actor->velocity += 0x20000;
    else if (position > 2)
        actor->velocity -= 0x20000;
    if (actor->velocity < 0) {
        actor->velocity += 0x8000;
        if (actor->velocity > 0)
            actor->velocity = 0;
    } else if (actor->velocity > 0) {
        actor->velocity -= 0x8000;
        if (actor->velocity < 0)
            actor->velocity = 0;
    }
    actor->position += actor->velocity;
    *(s16 *)(actor->model + 20) = *(s16 *)((u8 *)&actor->position + 2);
}
#else
#include "mine_actor_fields.h"

int mine_actor_motion(Obj8014A2E4 *actor, const MineActiveServices *services)
{
    if (!actor || !services)
        return 0;
    uint8_t *data = (uint8_t *)actor;
    uint32_t position = mine_actor_load32(data + 0xdc);
    uint32_t velocity = mine_actor_load32(data + 0xe0);
    int angle = mine_actor_signed16((uint16_t)(position >> 16));
    if (angle < -2)
        velocity += 0x20000;
    else if (angle > 2)
        velocity -= 0x20000;
    if (velocity & UINT32_C(0x80000000)) {
        velocity += 0x8000;
        if (!(velocity & UINT32_C(0x80000000)))
            velocity = 0;
    } else if (velocity) {
        velocity -= 0x8000;
        if (velocity & UINT32_C(0x80000000))
            velocity = 0;
    }
    position += velocity;
    mine_actor_store32(data + 0xe0, velocity);
    mine_actor_store32(data + 0xdc, position);
    if (!services->model)
        return 0;
    uint8_t *model = services->model(services->context, mine_actor_load32(data + 0x20));
    if (!model)
        return 0;
    mine_actor_store16(model + 20, (uint16_t)(position >> 16));
    return 1;
}
#endif
