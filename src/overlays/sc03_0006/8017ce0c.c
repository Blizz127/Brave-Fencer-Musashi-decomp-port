#include "forest_field.h"

typedef struct {
    u8 member;
    u8 needs_resource;
    u16 resource_id;
} CrystalResource;

typedef struct {
    u8 unused0[2];
    u16 phase;
    u8 unused4[0xd8];
    const CrystalResource *resource;
    u8 unusedE0[0x28];
    s16 resource_ready;
} CrystalLoader;


static inline s16 crystal_resource_ready(CrystalLoader *crystal)
{
    const CrystalResource *resource = crystal->resource;

    if (!resource->needs_resource) {
        return 1;
    }
    if (!crystal->resource_ready) {
        crystal->resource_ready = func_80128CFC(resource->resource_id);
    }
    return crystal->resource_ready;
}

ForestFieldPhase func_8017CE0C;

void func_8017CE0C(void *actor_slot)
{
    CrystalLoader *crystal = actor_slot;
    if (crystal_resource_ready(crystal)) {
        crystal->phase++;
    }
}
