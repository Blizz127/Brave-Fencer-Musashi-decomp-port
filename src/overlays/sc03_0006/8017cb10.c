#include "forest_shatter.h"

ForestFieldPhase func_8017CB10;

void func_8017CB10(void *actor_slot)
{
    FieldActor *actor = actor_slot;
    ForestShardVector *velocity = D_801E91DC.shards;
    ForestShardVector *shard = actor->shards;
    FieldEvent *event = actor->event;
    FieldLight *light = actor->light;
    u16 *model = actor->model;
    s16 i;

    if (event->uses_resource && !actor->resource_handle)
        actor->resource_handle = func_80128CFC(event->resource);
    light->size_x = light->size_y += 128;
    if (--actor->timer != -1) {
        func_8017C294(actor, 0);
        for (i = 0; i < 9; ++i, ++velocity, ++shard) {
            shard->x += velocity->x >> 1;
            if (i) {
                shard->y += (velocity->y + 48) >> 1;
                shard->z += velocity->z >> 1;
                shard->rx += 64;
                shard->ry += 128;
                shard->rz += 256;
            }
        }
    } else {
        *model = 0;
        ++actor->phase;
    }
}
