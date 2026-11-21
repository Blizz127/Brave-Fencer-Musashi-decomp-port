/* Original-disc reconstruction, SC01 member 77. Begin the actor's fall. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x10];
    u16 rotation_x;
    u16 rotation_y;
} FallModel;

typedef struct {
    u16 type;
    u16 state;
    u16 pad04, x, pad08, y, pad0c, z;
    u8 pad10[0x0c];
    s32 timer;
    u32 model;
    u8 pad24[0x10];
    u16 phase;
    u8 pad36[0x26];
    u16 flags;
    u8 pad5e[0x12];
    u8 variant;
    u8 pad71[0x6b];
    s32 offset, velocity, acceleration;
} FallingActor;

typedef struct {
    u16 x, y, z, type, selector, flags, aim, parameter;
    u32 user;
} FallPlacement;

extern s32 func_8012C51C(void *placement, u8 *owner);

void func_8017F5BC(u8 *data)
{
    FallingActor *actor = (FallingActor *)data;
    FallPlacement placement;
    FallingActor *spawned;
    FallModel *model = (FallModel *)(unsigned long)actor->model;

    actor->state = 2;
    actor->phase = 0;
    model->rotation_x = 0;
    actor->velocity = 0x100000;
    actor->acceleration = 0x40000;
    actor->offset = 0;
    actor->timer = 16;
    actor->flags &= 0x73ff;

    if (actor->variant != 4) {
        placement.x = actor->x + 240;
        placement.y = actor->y;
        placement.z = actor->z + 32;
        placement.type = 47;
        placement.selector = 0x600;
        placement.flags = 0;
        placement.user = 0;
        placement.parameter = 0;
        spawned = (FallingActor *)(unsigned long)(u32)func_8012C51C(&placement, data);
        if (spawned != 0) {
            ((FallModel *)(unsigned long)spawned->model)->rotation_y =
                ((FallModel *)(unsigned long)actor->model)->rotation_y;
        }
    }
}
