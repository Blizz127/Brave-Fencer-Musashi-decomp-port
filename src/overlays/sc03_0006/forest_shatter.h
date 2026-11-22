#ifndef SC03_0006_FOREST_SHATTER_H
#define SC03_0006_FOREST_SHATTER_H
#include "forest_field.h"

typedef struct {
    u16 active;
    u16 reserved02;
    u32 flags;
    u8 reserved08[0x18-0x08];
    s16 size_x, size_y;
    u8 reserved1c[0x34-0x1c];
    void *transform;
} FieldLight;

typedef struct {
    u8 index, uses_resource;
    u16 resource;
} FieldEvent;

typedef struct {
    u16 type, phase;
    s16 frac_x, x, frac_y, y, frac_z, z;
    u8 reserved10[0x1c-0x10];
    s32 timer;
    u16 *model;
    u8 reserved24[0xd0-0x24];
    FieldLight *light;
    u8 reservedd4[4];
    ForestShardVector *shards;
    FieldEvent *event;
    u8 reservede0[0x108-0xe0];
    s16 resource_handle;
} FieldActor;

typedef struct {
    u8 r, g, b, unused, size_x, size_y, size_z, pad;
} FieldColor;

extern FieldColor D_801F73F0;
extern ForestMatrix D_801F75E0;
#endif
