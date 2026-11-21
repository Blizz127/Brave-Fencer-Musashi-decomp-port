/* Owner re-derivation from retail assembly (clean input set). */
/* Returns the signed step that moves `current` toward `target` by at most `step` (the remaining distance when closer), or 0 when already there. */
#include "psx_types.h"

s16 func_80013154(s16 current, s16 target, s16 step) {
    s16 next;
    s32 down;

    if (current == target) {
        return 0;
    }
    down = target < current;
    if (down) {
        step = -step;
    }
    next = current + step;
    if (next >= target && !down) {
        return target - current;
    }
    if (next > target) {
        return step;
    }
    if (current < target) {
        return step;
    }
    return target - current;
}
