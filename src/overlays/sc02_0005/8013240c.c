#include "psx_types.h"

#if defined(__mips__) && !defined(__LP64__)
#define ANIM_VIEW
#define ANIM_HALF(value) (value)
#define ANIM_POINTER(type, address) ((type *)(address))
#else
#include <stdlib.h>
static s16 animation_half(s32 value)
{
    u32 bits = (u16)value;
    return (s16)((s32)(bits ^ 0x8000u) - 0x8000);
}
#define ANIM_HALF(value) animation_half(value)
/* These game records can occupy overlapping portions of one address space. */
#define ANIM_VIEW __attribute__((__may_alias__))
extern void *bfm_tower_animation_pointer(u32 address);
#define ANIM_POINTER(type, address) ((type *)bfm_tower_animation_pointer(address))
#endif

typedef struct ANIM_VIEW {
    u32 commands;
    s16 index;
    s16 elapsed;
    u32 model;
    u32 flags;
} AnimationState;

typedef struct ANIM_VIEW {
    u32 descriptor;
    s16 duration;
    s16 step;
} AnimationCommand;

typedef struct ANIM_VIEW {
    u32 first, second, third;
} AnimationInputs;

extern void func_801325B8(u32 model, u32 first, u32 second, u32 third, s32 weight);

#define ANIM_ROW(state) ANIM_POINTER(AnimationCommand, (state)->commands + (state)->index * 8)

void func_8013240C(void *context)
{
    AnimationState *state = (AnimationState *)context;
    AnimationCommand *command = ANIM_ROW(state);
    u32 key = command->descriptor;
    AnimationInputs *inputs;
    s32 weight;

    state->flags &= 0xffff3fffu;
    if (key == 0xffffffffu) {
        state->flags |= 0x4000;
    } else {
#if !defined(__mips__) || defined(__LP64__)
        /* The original faults on division by zero for a live command. */
        if (!command->duration) abort();
#endif
        if (command->step < 0) {
            weight = 4096 - (state->elapsed * 4096 / command->duration);
        } else {
            weight = state->elapsed * 4096 / command->duration;
        }
        inputs = ANIM_POINTER(AnimationInputs, key);
        func_801325B8(state->model, inputs->first, inputs->second, inputs->third, weight);
        if (command->step < 0) {
            state->elapsed = ANIM_HALF(state->elapsed - command->step);
        } else {
            state->elapsed = ANIM_HALF(state->elapsed + command->step);
        }
        if (state->elapsed >= command->duration) {
            u32 next;
            state->elapsed = 0;
            state->index = ANIM_HALF(state->index + 1);
            next = ANIM_ROW(state)->descriptor;
            if (next != 0xffffffffu) {
                if (!next) {
                    state->index = 0;
                    state->flags |= 0x8000;
                }
            } else {
                state->flags |= 0x4000;
            }
        }
    }
}
