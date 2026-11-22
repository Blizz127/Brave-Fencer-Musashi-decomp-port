/* Follow the selected target, then wait for the event handle or the fallback countdown. */
#include "mine_award_middle.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define TARGET_POOL() (*(u32 *)&D_80127508)
#else
#define TARGET_POOL() D_80127508
#endif

void func_80173E1C(u8 *actor)
{
    u8 vectors[16];

    if (TARGET_POOL()) {
        func_801484B0(vectors, vectors + 8);
        func_80149350(actor, vectors, vectors);
        func_80015954(vectors, (u8 *)TARGET_POOL() + 4);
    }
    if (READ_HANDLE(actor)) {
        if (func_801399F0(READ_HANDLE(actor))) {
            func_80139914(READ_HANDLE(actor));
            WRITE_HANDLE(actor, 0);
            func_80154274(actor, D_800D5DC0);
            func_80171A1C(actor);
        }
    } else {
        u32 count = READ_COUNT(actor) - 1u;
        WRITE_COUNT(actor, count);
        if (count == (u32)-1) {
            func_80154274(actor, D_800D5DC0);
            func_80171A1C(actor);
        }
    }
}
