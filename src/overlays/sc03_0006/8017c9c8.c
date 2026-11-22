#include "forest_shatter.h"

ForestFieldPhase func_8017C9C8;

void func_8017C9C8(void *actor_slot)
{
    FieldActor *actor = actor_slot;
    ForestVector position, rotation;
    FieldLight *light = func_8001D074(0x7e, 0x100);

    if (light != 0) {
        actor->light = light;
        func_8001CD50(light, &D_801F73F0);
        D_801F73F0.r = D_801F73F0.b = D_801F73F0.g = 255;
        D_801F73F0.size_x = D_801F73F0.size_z = 8;
        D_801F73F0.size_y = 24;
        func_800233CC(&D_801F73F0, 0x100);
        position.x = actor->x;
        position.y = actor->y - 56;
        position.z = actor->z;
        func_800D20C0(&position, &rotation, 5);
        D_801F75E0.position[0] = position.x;
        D_801F75E0.position[1] = position.y;
        D_801F75E0.position[2] = position.z;
        func_800D23D0(&rotation);
        func_80049A1C(&rotation, &D_801F75E0);
        light->transform = &D_801F75E0;
        light->size_x = light->size_y = 0;
        light->flags |= 0x50000000;
        ++actor->phase;
    }
}
