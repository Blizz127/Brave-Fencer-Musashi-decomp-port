/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Starts a pitch slide toward the target/speed pair chosen by the channel's envelope stage type (absolute or relative to its base pitch). */
#include "psx_types.h"

typedef struct {
    u16 type;
    u16 pad2[2];
    u16 value[5];
} PitchStage;

typedef struct {
    u8 pad0[0x14];
    u32 stage; /* guest PitchStage * */
    u8 pad18[0x20 - 0x18];
    s32 base;
    s32 current;
    s32 target;
    s32 step;
    u8 pad30[0x37 - 0x30];
    u8 flags;
    u8 done;
} PitchSlide;

void func_80031988(PitchSlide *slide) {
    PitchStage *stage;
    u16 target;
    u16 speed;

    stage = (PitchStage *)slide->stage;
    if (stage == 0) {
        return;
    }
    switch (stage->type) {
    case 0:
        target = stage->value[0];
        speed = stage->value[1];
        break;
    case 7:
        target = stage->value[0] + slide->base;
        speed = stage->value[1];
        break;
    case 5:
        target = stage->value[1];
        speed = stage->value[2];
        break;
    case 8:
        target = stage->value[1] + slide->base;
        speed = stage->value[2];
        break;
    case 1:
        target = stage->value[2];
        speed = stage->value[3];
        break;
    case 9:
        target = stage->value[2] + slide->base;
        speed = stage->value[3];
        break;
    case 6:
        target = stage->value[3];
        speed = stage->value[4];
        break;
    case 10:
        target = stage->value[3] + slide->base;
        speed = stage->value[4];
        break;
    default:
        return;
    }
    if (slide->current != target) {
        slide->done = 0;
        slide->target = target;
        slide->flags |= 1;
        if (target < slide->current) {
            slide->step = -speed;
        } else {
            slide->step = speed;
        }
    }
}
