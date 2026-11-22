/* Select a player transition, or advance the current animation state. */
#include "../../../include/forest_action.h"

void func_80159C84(ForestActionPlayer *actor)
{
    u32 alternate;

    if (func_80149D9C(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80149FA8(actor);
        return;
    }
    func_80149FB0(actor);
    if (func_80161B18(actor) || func_80161B84(actor) || func_80161BE0(actor))
        return;
    if (func_801496D4(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80149704(actor);
        return;
    }
    switch (func_801619A4(actor)) {
    case 0:
    case 1:
    case 2:
    case 6:
        if (func_80149AA8(actor)) {
            FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
            func_80149AD4(actor);
            return;
        }
        if (func_80149B54(actor)) {
            FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
            if (func_8014A738(actor))
                func_8014A830(actor);
            else
                func_80149BAC(actor);
            return;
        }
        break;
    case 3:
        func_801498E0(FOREST_ACTOR_GAME_ADDRESS(actor));
        break;
    case 4:
        func_80149954(actor);
        break;
    case 5:
        if (func_80149A64(actor)) {
            FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
            func_800CCCC0(actor);
            return;
        }
        break;
    }
    if (func_80149CD4(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_8015E880(actor);
        return;
    }
    if (func_801488A8(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_8015A230(actor);
        return;
    }

    if (!(actor->flags & 0x200) && actor->sequence_flags == 0x8000) {
        if ((s32)actor->step >= 3) {
            alternate = ACTION_DATA(800D5F44);
            if (actor->sequence == alternate)
                func_80154B4C(actor, ACTION_DATA(800D5F34));
            else if ((ACTION_SAMPLE() & 15) == 0)
                func_80154B4C(actor, alternate);
        } else {
            alternate = ACTION_DATA(800D5F14);
            if (actor->sequence == alternate)
                func_80154B4C(actor, ACTION_DATA(800D5F04));
            else if ((ACTION_SAMPLE() & 15) == 0)
                func_80154B4C(actor, alternate);
        }
    }

    if (actor->animation_flags & 0x8000) {
        if (!(actor->flags & 0x200)) {
            if (func_8016F1AC()) {
                actor->step = 0;
                actor->counter = 0;
            }
            switch (actor->step) {
            case 0:
                if ((s32)actor->counter++ >= 4) {
                    actor->counter = 0;
                    actor->step++;
                    func_80154150(actor, 1);
                } else {
                    ACTION_VECTOR(actor, ACTION_DATA(8018911C));
                    func_80154190(actor, 0);
                }
                break;
            case 1:
                actor->counter = 0;
                actor->step++;
                func_80154150(actor, 0);
                break;
            case 2:
                if ((s32)actor->counter++ >= 2) {
                    if (actor->flags & 4) {
                        actor->step = 0;
                        actor->counter = 0;
                        func_80154150(actor, 0);
                    } else {
                        actor->counter = 0;
                        actor->step++;
                        func_80154150(actor, 2);
                    }
                } else
                    func_80154190(actor, 0);
                break;
            case 3:
                func_80147324(0x445);
                actor->step++;
                func_80155440(actor);
                func_80154150(actor, 3);
                func_80154A74(actor, 0x13);
                break;
            case 4:
                break;
            }
        } else {
            ACTION_VECTOR(actor, ACTION_DATA(8018911C));
            func_80154150(actor, 5);
            func_80154A74(actor, 0x1c);
        }
    }
    if (actor->animation == ACTION_DATA(800D49F4)) {
        func_80147A84(actor);
        func_801473EC(actor);
        if (!(D_800B99DA & 3))
            func_8014C010((s32)FOREST_ACTOR_GAME_ADDRESS(actor), 1);
    }
    if (func_80161208(actor) == 0) {
        func_801483E8(actor);
        func_80172590(actor);
    }
}
