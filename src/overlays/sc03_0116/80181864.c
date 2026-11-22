#include "actor_33a_intensity.h"

s32 func_80181864(Actor33A *actor, struct Ring *shape)
{
    s32 intensity = READ_SIGNED_TIMER(actor);
    s32 next;

    if (D_800B99DA & 1)
        intensity -= 4;
    if (intensity < 0)
        intensity = 0;
    else if (intensity > 255)
        intensity = 255;
#if defined(__mips__) && !defined(__LP64__)
    ((Actor33ARing *)shape)->control.intensity = (u32)intensity;
    ((Actor33ARing *)shape)->control.secondary = 0;
#else
    {
        u32 words[2] = {(u32)intensity, 0};
        memcpy(shape, words, sizeof(words));
    }
#endif
    next = READ_TIMER(actor) + 4;
    WRITE_TIMER(actor, next);
    return next;
}
