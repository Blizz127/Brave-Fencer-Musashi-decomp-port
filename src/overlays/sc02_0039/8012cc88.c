#include "mine_actor_step.h"

#if defined(__mips__) && !defined(__LP64__)
#define STEP_OFFSET(vector, field) ((vector)->field)
#else
#include <stddef.h>
#include <string.h>

/* The offset can refer to the actor's own position or velocity halfwords. */
static s16 read_step_offset(const void *vector, size_t field)
{
    s16 value;
    memcpy(&value, (const unsigned char *)vector + field, sizeof(value));
    return value;
}
#define STEP_OFFSET(vector, field) read_step_offset((vector), offsetof(SceneProbeVector, field))
#endif

s32 func_8012CC88(MineStepActor *actor, s32 mode, SceneProbeVector *offset)
{
    MineActionPoint from, to;
    s32 result;

    from.x = actor->previous[0].half.integer;
    from.y = actor->previous[1].half.integer;
    from.z = actor->previous[2].half.integer;
    from.x += STEP_OFFSET(offset, x);
    from.y += STEP_OFFSET(offset, y);
    from.z += STEP_OFFSET(offset, z);

    actor->velocity[0] += actor->acceleration[0];
    actor->velocity[1] += actor->acceleration[1];
    actor->velocity[2] += actor->acceleration[2];
    actor->position[0].value += actor->velocity[0];
    actor->position[1].value += actor->velocity[1];
    actor->position[2].value += actor->velocity[2];

    to.x = actor->position[0].half.integer;
    to.y = actor->position[1].half.integer;
    to.z = actor->position[2].half.integer;
    to.x += STEP_OFFSET(offset, x);
    to.y += STEP_OFFSET(offset, y);
    to.z += STEP_OFFSET(offset, z);
    result = func_8012CEB0(&from, &to, mode);
    to.x -= STEP_OFFSET(offset, x);
    to.y -= STEP_OFFSET(offset, y);
    to.z -= STEP_OFFSET(offset, z);
    actor->position[0].half.integer = to.x;
    actor->position[1].half.integer = to.y;
    actor->position[2].half.integer = to.z;
    if (result & 0x6000)
        actor->velocity[1] = 0;
    return result;
}
