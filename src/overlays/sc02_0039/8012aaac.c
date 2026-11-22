#include "mine_animation_apply.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
typedef struct {
    u32 value; /* +0: frame value, or the target sequence of a jump/call */
    s16 op;    /* +4: >= 0 hold time, < 0 control code */
    u8 pad6[2];
} Frame8012AAAC; /* 8 bytes */

typedef struct {
    u8 pad00[0x20];
    u32 target;    /* 0x20: guest pointer; the frame value goes to its +0x20 */
    u8 pad24[0x4E];
    u16 flags;     /* 0x72 */
    u8 pad74[0x1C];
    u32 frames;    /* 0x90: guest pointer to Frame8012AAAC[] */
    s32 index;     /* 0x94 */
    s16 delay;     /* 0x98 */
    s16 retIndex;  /* 0x9A: index to resume at after a call */
    u32 retFrames; /* 0x9C: guest pointer to the calling sequence, 0 if none */
} Obj8012AAAC;

#define command_actor ((Obj8012AAAC *)actor)
void func_8012AAAC(Obj8014A2E4 *actor)
{
    Frame8012AAAC *f;
    u32 saved;
    s32 next;

    command_actor->flags &= 0xBFFF;
    f = &((Frame8012AAAC *)command_actor->frames)[command_actor->index];
    if (f->op < 0) {
        switch (f->op) {
        case -2:
            if (command_actor->retFrames != 0) {
                command_actor->frames = command_actor->retFrames;
                command_actor->index = command_actor->retIndex;
                command_actor->retFrames = 0;
                command_actor->retIndex = 0;
                func_8012AAAC(actor);
            } else {
                command_actor->delay = 0;
            }
            break;
        case -1:
            if (command_actor->retFrames != 0) {
                command_actor->frames = command_actor->retFrames;
                command_actor->index = command_actor->retIndex;
                command_actor->retFrames = 0;
                command_actor->retIndex = 0;
                func_8012AAAC(actor);
            } else {
                command_actor->index = 0;
                func_8012AAAC(actor);
                command_actor->flags |= 0x4000;
            }
            break;
        case -4:
            command_actor->frames = f->value;
            command_actor->index = 0;
            command_actor->retFrames = 0;
            command_actor->retIndex = 0;
            command_actor->flags &= 0xF9FF;
            func_8012AAAC(actor);
            break;
        case -3:
            next = command_actor->index + 1;
            command_actor->index = next;
            if (command_actor->retFrames != 0) {
                func_8012AAAC(actor);
            } else {
                saved = command_actor->frames;
                command_actor->frames = f->value;
                command_actor->index = 0;
                command_actor->retFrames = 0;
                command_actor->retIndex = 0;
                command_actor->flags &= 0xF9FF;
                func_8012AAAC(actor);
                command_actor->retFrames = saved;
                command_actor->retIndex = next;
            }
            break;
        case -50:
            command_actor->index++;
            func_8012AAAC(actor);
            command_actor->flags |= 0x2000;
            break;
        case -5:
            command_actor->flags |= 0x400;
            command_actor->index--;
            func_8012AAAC(actor);
            break;
        }
    } else {
        *(u32 *)(command_actor->target + 0x20) = f->value;
        command_actor->delay = f->op;
    }
}

#undef command_actor
#else
#include <stdint.h>
#include <string.h>

static u16 half(const u8 *p)
{
    return (u16)(p[0] | (u16)p[1] << 8);
}

static u32 word(const u8 *p)
{
    return (u32)half(p) | (u32)half(p + 2) << 16;
}

static void put_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}

static void put_word(u8 *p, u32 value)
{
    put_half(p, (u16)value);
    put_half(p + 2, (u16)(value >> 16));
}

enum {
    target_key = 0x20,
    flags = 0x72,
    frame_key = 0x90,
    index = 0x94,
    delay = 0x98,
    resume_index = 0x9a,
    resume_key = 0x9c
};

void func_8012AAAC(Obj8014A2E4 *actor)
{
    Obj8014A2E4 saved_actor;
    u8 *a = (u8 *)actor;
    u16 pending_flags = 0, outer_index = 0;
    u32 outer_key = 0, address = 0;
    u32 remaining;
    unsigned reason = BFM_ANIMATION_BAD_FRAME;

    if (!actor) {
        bfm_animation_command_refused(actor, BFM_ANIMATION_BAD_ACTOR, 0);
        return;
    }
    memcpy(&saved_actor, actor, sizeof(saved_actor));
    remaining = bfm_animation_command_budget(actor);
    for (;;) {
        u32 key = word(a + frame_key), i = word(a + index);
        s16 op;
        u8 *value, *destination;
        u32 last_value, next;

        if (!remaining) {
            reason = BFM_ANIMATION_COMMAND_LIMIT;
            break;
        }
        remaining--;
        put_half(a + flags, half(a + flags) & 0xbfff);
        /* The indexed address wraps in the PS1 address register. */
        address = key + i * 8u;
        if (!key || !address || address > UINT32_MAX - 5u || (address & 1u))
            break;
        if (!bfm_animation_frame_operation(address + 4u, &op))
            break;
        if (op >= 0) {
            u32 target = word(a + target_key);
            if ((address & 3u) || !(value = bfm_animation_word_view(address, 0)))
                break;
            next = word(value);
            if (!target || (target & 3u) || target > UINT32_MAX - 0x23u ||
                !(destination = bfm_animation_word_view(target + 0x20u, 1))) {
                reason = BFM_ANIMATION_BAD_TARGET;
                address = target;
                break;
            }
            last_value = word(destination);
            put_word(destination, next);
            /* A target may alias the frame. The original reads the delay again. */
            if (!bfm_animation_frame_operation(address + 4u, &op)) {
                put_word(destination, last_value);
                break;
            }
            put_half(a + delay, (u16)op);
            goto complete;
        }
        switch (op) {
        case -50:
            put_word(a + index, i + 1u);
            pending_flags |= 0x2000;
            break;
        case -5:
            put_half(a + flags, half(a + flags) | 0x400);
            put_word(a + index, i - 1u);
            break;
        case -4:
        case -3:
            if (op == -3) {
                put_word(a + index, i + 1u);
                if (word(a + resume_key))
                    break;
                /* These two stores occur on return. The outermost call wins. */
                if (!outer_key) {
                    outer_key = key;
                    outer_index = (u16)(i + 1u);
                }
            }
            if ((address & 3u) || !(value = bfm_animation_word_view(address, 0)))
                goto refused;
            next = word(value);
            put_word(a + frame_key, next);
            put_word(a + index, 0);
            put_word(a + resume_key, 0);
            put_half(a + resume_index, 0);
            put_half(a + flags, half(a + flags) & 0xf9ff);
            break;
        case -2:
        case -1:
            next = word(a + resume_key);
            if (next) {
                u16 resume = half(a + resume_index);
                put_word(a + frame_key, next);
                put_word(a + resume_key, 0);
                put_half(a + resume_index, 0);
                put_word(a + index, (u32)((s32)(resume ^ 0x8000u) - 0x8000));
            } else if (op == -2) {
                put_half(a + delay, 0);
                goto complete;
            } else {
                put_word(a + index, 0);
                pending_flags |= 0x4000;
            }
            break;
        default:
            goto complete;
        }
    }
refused:
    memcpy(actor, &saved_actor, sizeof(saved_actor));
    bfm_animation_command_refused(actor, reason, address);
    return;
complete:
    if (outer_key) {
        put_word(a + resume_key, outer_key);
        put_half(a + resume_index, outer_index);
    }
    if (pending_flags)
        put_half(a + flags, half(a + flags) | pending_flags);
}
#endif
