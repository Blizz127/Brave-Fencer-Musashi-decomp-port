#include "camera.h"

extern s16 func_80012C6C(s16 from, s16 to, s16 rate);
extern s32 func_80012ABC(s32 from, s16 to, s16 rate);
extern void func_8017D930(SceneVector *angles, s32 *transform);
extern SceneVector *func_80048FBC(s32 *transform, SceneVector *input, SceneVector *result);

void func_8017D2B8(FacingActor *actor, s16 facing, SceneVector *position)
{
    s32 transform[8];
    SceneVector input;
    SceneVector result;
    SceneVector rotation;
    actor->distance = func_80012C6C(actor->distance, actor->target_distance, 4);
    actor->depth = func_80012C6C(actor->depth, actor->phase_value, 4);
    actor->rotation.position.x = func_80012ABC(actor->rotation.position.x, actor->target_rotation.position.x, 4);
    actor->rotation.position.y = func_80012ABC(actor->rotation.position.y, actor->target_rotation.position.y, 4);
    actor->rotation.position.z = func_80012ABC(actor->rotation.position.z, actor->target_rotation.position.z, 4);
    actor->offset.x = func_80012C6C(actor->offset.x, actor->target_offset.x, 16);
    actor->offset.y = func_80012C6C(actor->offset.y, actor->target_offset.y, 16);
    actor->offset.z = func_80012C6C(actor->offset.z, actor->target_offset.z, 16);
    actor->focus.x = actor->offset.x + position->position.x;
    actor->focus.y = actor->offset.y + position->position.y;
    actor->focus.z = actor->offset.z + position->position.z;
    rotation.position.x = actor->rotation.position.x;
    rotation.position.y = actor->rotation.position.y + facing;
    rotation.position.z = actor->rotation.position.z;
    func_8017D930(&rotation, transform);
    input.position.x = 0;
    input.position.y = 0;
    input.position.z = actor->depth;
    func_80048FBC(transform, &input, &result);
    actor->camera.x = (result.position.x >> 3) + actor->offset.x + position->position.x;
    actor->camera.y = (result.position.y >> 3) + actor->offset.y + position->position.y;
    actor->camera.z = (result.position.z >> 3) + actor->offset.z + position->position.z;
}
