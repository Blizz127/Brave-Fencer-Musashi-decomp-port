/* Advance an actor's animation timer and interpret the command at its cursor. */
#include "../../../include/forest_animation_commands.h"

static __inline__ void finish_animation_frame(ForestPlayerState *actor)
{
    actor->animation_flags = actor->animation_duration;
    ++actor->animation_cursor;
    ++actor->animation_step;
    ++actor->animation_frame;
}

static __inline__ void mark_animation_step(ForestPlayerState *actor, u32 *started,
                                           u32 *flags, u32 *suppress, u32 mask)
{
    actor->animation_flags = 1;
    CONTROL_WRITE(flags, CONTROL_READ(flags) | mask);
    ++actor->animation_cursor;
    ++actor->animation_step;
    func_8015444C(actor, started, flags, suppress);
}

void func_8015444C(ForestPlayerState *actor, u32 *started, u32 *flags, u32 *suppress)
{
    u32 model, command, cursor, operand;

    ANIMATION_ENTER();
    model = actor->model;
    if (!model || !ANIMATION_COUNTDOWN(actor))
        return;
    if (!CONTROL_READ(started)) {
        actor->animation_flags = ANIMATION_COUNTDOWN(actor);
        CONTROL_WRITE(started, CONTROL_READ(started) + 1);
    }
    --actor->animation_flags;
    if (!(u8)actor->animation_flags) {
        command = ANIMATION_WORD(actor->animation, actor->animation_cursor);
        switch (command) {
        case 0:
            actor->animation_flags = 0;
            CONTROL_WRITE(flags, CONTROL_READ(flags) | 0x8000);
            actor->animation_duration = 0;
            actor->animation_cursor = 0;
            actor->animation_step = 0;
            break;
        case 1:
            actor->animation_flags = 1;
            CONTROL_WRITE(flags, CONTROL_READ(flags) | 0x8000);
            actor->animation_cursor = 0;
            actor->animation_step = 0;
            actor->animation_frame = 0;
            func_8015444C(actor, started, flags, suppress);
            break;
        case 2:
            actor->animation_flags = 0;
            ++actor->animation_cursor;
            CONTROL_WRITE(flags, CONTROL_READ(flags) | 0x8000);
            break;
        case 3:
            operand = ANIMATION_WORD(actor->animation, actor->animation_cursor + 1);
            actor->animation_flags = 1;
            actor->animation_cursor += 2;
            actor->animation_duration = (u16)operand;
            func_8015444C(actor, started, flags, suppress);
            break;
        case 4:
        case 11:
        case 12:
            goto skip_operand;
        case 5:
            operand = ANIMATION_WORD(actor->animation, actor->animation_cursor + 1);
            cursor = ANIMATION_WORD(actor->animation, actor->animation_cursor + 2);
            actor->animation_flags = 1;
            actor->animation_frame = 0;
            actor->animation_cursor = (u8)cursor;
            actor->animation = operand;
            func_8015444C(actor, started, flags, suppress);
            break;
        case 6:
            cursor = ANIMATION_WORD(actor->animation, actor->animation_cursor + 1);
            actor->animation_flags = 1;
            actor->animation_cursor = (u8)cursor;
            actor->animation_frame = (u8)func_801549F8(actor);
            func_8015444C(actor, started, flags, suppress);
            break;
        case 7:
            mark_animation_step(actor, started, flags, suppress, 0x4000);
            break;
        case 8:
            mark_animation_step(actor, started, flags, suppress, 0x2000);
            break;
        case 9:
            mark_animation_step(actor, started, flags, suppress, 0x1000);
            break;
        case 10:
            mark_animation_step(actor, started, flags, suppress, 0x0800);
            break;
        case 13:
            actor->animation_flags = 1;
            ++actor->animation_cursor;
            func_8015444C(actor, started, flags, suppress);
            break;
        case 14:
            if (!CONTROL_READ(suppress))
                func_80154B4C(actor, ANIMATION_WORD(actor->animation, actor->animation_cursor + 1));
            goto skip_operand;
        case 15:
            if (!CONTROL_READ(suppress))
                func_80154A74(actor, (s32)ANIMATION_WORD(actor->animation, actor->animation_cursor + 1));
            goto skip_operand;
        case 16:
            if (!CONTROL_READ(suppress))
                func_801553C0(actor);
            goto advance_command;
        case 17:
            if (!CONTROL_READ(suppress))
                func_80155440(actor);
            goto advance_command;
        case 18:
            if (!CONTROL_READ(suppress))
                func_80155394(actor);
            goto advance_command;
        case 19:
            if (!CONTROL_READ(suppress))
                func_801553A8(actor);
            goto advance_command;
        case 20:
            if (!CONTROL_READ(suppress))
                actor->field_1a8 ^= 1;
            goto advance_command;
        case 21:
            if (!CONTROL_READ(suppress))
                ANIMATION_RESOURCE(ANIMATION_WORD(actor->animation, actor->animation_cursor + 1), 80126738);
            goto skip_operand;
        case 22:
            if (!CONTROL_READ(suppress))
                ANIMATION_RESOURCE(ANIMATION_WORD(actor->animation, actor->animation_cursor + 1), 80126838);
        skip_operand:
            actor->animation_flags = 1;
            actor->animation_cursor += 2;
            func_8015444C(actor, started, flags, suppress);
            break;
        case 23:
            if (!CONTROL_READ(suppress))
                ANIMATION_MODEL_FRAME(model, ANIMATION_ADDRESS(80126748));
            finish_animation_frame(actor);
            break;
        case 24:
            if (!CONTROL_READ(suppress))
                ANIMATION_MODEL_FRAME(model, ANIMATION_ADDRESS(80126848));
            finish_animation_frame(actor);
            break;
        case 25:
            if (!CONTROL_READ(suppress))
                actor->flags &= ~0x20000u;
            goto advance_command;
        case 26:
            if (!CONTROL_READ(suppress))
                actor->flags |= 0x20000u;
        advance_command:
            actor->animation_flags = 1;
            ++actor->animation_cursor;
            ++actor->animation_step;
            func_8015444C(actor, started, flags, suppress);
            break;
        default:
            ANIMATION_MODEL_FRAME(model, command + 16u);
            finish_animation_frame(actor);
            break;
        }
    }
    actor->animation_flags |= CONTROL_READ(flags);
}
