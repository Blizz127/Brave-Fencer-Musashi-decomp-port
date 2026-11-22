/* Ordinary forest player movement, collision and action transitions. */
#include "forest_player_locomotion.h"

/* The original action callback may leave its full signed result in v0. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define FOREST_GCC2_ABI 1
#else
#define FOREST_GCC2_ABI 0
#endif

s32 func_8015A3C8(ForestPlayerState *actor)
{
    s32 minimum, acceleration, previous, result, intermediate;
    ForestMode1Stats *formation = &D_80078E78;
#if FOREST_GCC2_ABI
    register s32 limit __asm__("$16");
#else
    s32 limit;
#endif
    if (D_800B9A08 == 0x302A) {
        minimum = -0xC0000;
        acceleration = -0x4000;
    } else {
        minimum = -0xB0000;
        acceleration = -0x1000;
    }
#if FOREST_GCC2_ABI
    /* Keep the zero speed ceiling in the original callee-saved register. */
    __asm__("" : "=r"(limit) : "0"(0) : "$4");
#else
    limit = 0;
#endif
    if (func_80149D9C(actor)) {
        result = FOREST_ACTION_RESULT(801891B8, actor->kind, actor);
        func_80149FA8(actor);
#if FOREST_GCC2_ABI
        {
            register s32 retained __asm__("$2");
            __asm__("" : "=r"(retained));
            return retained;
        }
#else
        return result;
#endif
    }
    func_80149FB0(actor);
    result = func_80161B84(actor);
    if (result)
        return result;
    result = func_80161B18(actor);
    if (result)
        return result;
    if (actor->animation == ANIMATION_ADDRESS && func_8014A048(actor)) {
        DISPATCH_KIND(actor);
        return func_8014A218(actor);
    }
    result = func_80161BE0(actor);
    if (result)
        return result;
    switch (func_801619A4(actor)) {
    case 0:
    case 1:
    case 2:
    case 6:
        if (func_80149AA8(actor)) {
            DISPATCH_KIND(actor);
            return func_80149AD4(actor);
        }
        if (func_80149B54(actor)) {
            DISPATCH_KIND(actor);
            return func_80149BAC(actor);
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
            DISPATCH_KIND(actor);
            return func_800CCCC0(actor);
        }
        break;
    }
    if (func_80149CD4(actor)) {
        DISPATCH_KIND(actor);
        return func_8015E880(actor);
    }
    if (func_801496D4(actor)) {
        DISPATCH_KIND(actor);
        return func_80149704(actor);
    }
    if (!func_801488A8(actor)) {
        previous = actor->field_2c.word;
        DISPATCH_KIND(actor);
        func_8014C010(FOREST_ACTOR_GAME_ADDRESS(actor), 1);
        func_80147078(actor, 0);
        func_80159B70(actor);
#if FOREST_GCC2_ABI
        {
            register s32 raw __asm__("$2") = func_8015ABD4(FOREST_ACTOR_GAME_ADDRESS(actor), previous, minimum);
            register s32 retained __asm__("$2");
            __asm__("" : "=r"(retained) : "0"(raw));
            if (!raw)
                return retained;
            return func_8015AC48(actor);
        }
#else
        if (!func_8015ABD4(FOREST_ACTOR_GAME_ADDRESS(actor), previous, minimum))
            return 0;
        return func_8015AC48(actor);
#endif
    }
    func_80148AFC(actor);
    if (actor->flags & 0x200) {
        if (!actor->field_23c) {
            func_80154150(actor, 10);
            func_80154A74(actor, 0x1c);
        }
        MOVEMENT_VECTOR(actor);
        func_80147AD4(actor, 0, 0, 0);
    } else {
        switch (FOREST_MOVEMENT_CONTROLLER(actor)) {
        case 0x41:
            func_80147AD4(actor, 0, 0, acceleration);
            if (actor->field_23c)
                break;
            if (func_801487F4(actor) & 4) {
                if (actor->field_2c.word < -0x40000)
                    actor->field_2c.word = -0x40000;
            } else if (!func_8016F1C4()) {
                if (formation->stat26 < 32 || formation->stat2e < 32) {
                    if (actor->field_2c.word < -0x40000)
                        actor->field_2c.word = -0x40000;
                } else if (formation->stat26 < 64 || formation->stat2e < 48) {
                    if (actor->field_2c.word < -0x80000)
                        actor->field_2c.word = -0x80000;
                }
            }
            if (actor->field_2c.word < minimum)
                actor->field_2c.word = minimum;
            if (actor->field_2c.word >= limit)
                actor->field_2c.word = limit;
            if (actor->field_2c.word < -0x94000)
                func_80154150(actor, 8);
            else if (actor->field_2c.word < -0x70000) {
                intermediate = func_8015AB7C(actor);
                func_801541D8(actor, 7, (u16)intermediate);
            } else
                func_80154150(actor, 6);
            break;
        case 0x53:
        case 0x73:
            actor->field_2c.word = minimum * (u8)func_80148824(actor) / 127;
            if (!func_8016F1C4()) {
                if ((formation->stat26 < 64 || formation->stat2e < 48) &&
                    actor->field_2c.word < -0x80000)
                    actor->field_2c.word = -0x80000;
                if ((formation->stat26 < 32 || formation->stat2e < 32) &&
                    actor->field_2c.word < -0x40000)
                    actor->field_2c.word = -0x40000;
            }
            if (actor->field_2c.word < minimum)
                actor->field_2c.word = minimum;
            if (actor->field_2c.word >= limit)
                actor->field_2c.word = limit;
            func_80147A84(actor);
            if (actor->field_23c)
                break;
            if (actor->field_2c.word < -0x94000) {
                func_80154150(actor, 8);
                break;
            }
            if (actor->field_2c.word < -0x70000) {
                intermediate = func_8015AB7C(actor);
                func_801541D8(actor, 7, (u16)intermediate);
            } else {
                func_80154150(actor, 6);
            }
            break;
        }
    }
    func_801473EC(actor);
    if (minimum >= actor->field_2c.word && !(D_800B99DA & 7))
        func_8014C010(FOREST_ACTOR_GAME_ADDRESS(actor), 2);
    if (func_8014D738(actor)) {
        DISPATCH_KIND(actor);
        return func_8015C6E0(actor);
    }
    if (func_8014E934(actor)) {
        func_80155440(actor);
        func_80154150(actor, 0x12);
        func_80154A74(actor, 0x17);
        actor->field_23c = 1;
    } else {
        if (actor->field_23c) {
            func_801553C0(actor);
            func_80154150(actor, 6);
            func_80154A74(actor, 0x21);
        }
        actor->field_23c = 0;
    }
    result = func_80161278(actor);
    if (result)
        return result;
    if (actor->animation_flags & 0x2000)
        func_80161C24(actor, actor->field_16e);
    func_80172608(actor);
    return func_801483E8(actor);
}
