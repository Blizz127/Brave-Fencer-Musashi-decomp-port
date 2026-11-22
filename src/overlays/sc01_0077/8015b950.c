/* Evaluate the transition for kind 2, action 2 and update its motion state. */
#include "../../../include/forest_kind2_phase.h"

static __inline__ s32 below_motion_limit(s32 value, s32 lower)
{
    if (!func_8016F1AC()) {
        if (value <= lower)
            return 1;
        if (D_8011F9C4 && !(u8)func_80029178(0x21))
            return 1;
    }
    return 0;
}

static __inline__ s32 signed_coordinate(const s16 *position)
{
    return *position;
}

void func_8015B950(ForestPlayerState *actor)
{
    s16 zero[4], captured[4];
    s32 value;
    u32 distance_limit;
    s32 lower = -0xb0000;

    if (func_80161B18(actor))
        return;
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
    if (func_80149744(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80149788(actor);
        return;
    }
    if (func_801496D4(actor)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80149704(actor);
        if (!(u8)func_80148AFC(actor))
            func_80146D90(actor);
        if (actor->field_24.word | actor->field_2c.word)
            func_80154150(actor, 15);
        else
            func_801541D8(actor, 13, 6);
        actor->counter = 1;
        return;
    }
    if (func_801488A8(actor)) {
        zero[0] = 0;
        zero[1] = 0;
        zero[2] = 0;
        captured[0] = signed_coordinate(&actor->field_24.halves.high);
        captured[1] = 0;
        captured[2] = signed_coordinate(&actor->field_2c.halves.high);
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80147078(actor, 1);
        func_8015A264(actor);
        distance_limit = 0u - ((u32)FOREST_DISTANCE(zero, captured) << 16);
        if ((s32)distance_limit < actor->field_2c.word)
            actor->field_2c.word = (s32)distance_limit;
        return;
    }
    if (actor->animation_flags == 0x8000) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80147078(actor, 0);
        func_80159B70(actor);
        return;
    }
    value = actor->field_2c.word;
    if (below_motion_limit(value, lower)) {
        FOREST_PLAYER_CALL(801891B8, actor->kind, actor);
        func_80147078(actor, 0);
        func_80159B70(actor);
        func_80154150(actor, 9);
        ACTION_VECTOR(actor, ACTION_DATA(8018914C));
        func_80161CD0(actor, actor->field_16e);
        return;
    }
    if (!func_80161208(actor))
        func_801725A4(actor);
}
