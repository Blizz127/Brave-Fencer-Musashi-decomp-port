/* Find the first pool entry with the requested unsigned type and signed identifier. */
#include "mine_award_controller.h"
#include "../main_0012/scene_object_probes.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define LOOKUP_RESULT s32
#define LOOKUP_CONTROLLER s32
#define ACTOR_ID(actor) (*(s16 *)((u8 *)(actor) + 0x70))
#else
#include "mine_actor_fields.h"
#define LOOKUP_RESULT void *
#define LOOKUP_CONTROLLER u8 *

static s32 mine_actor_identifier(const Obj8014A2E4 *actor)
{
    const u8 *field = actor->pad05E + (0x70 - 0x5e);
    return mine_actor_signed16(mine_actor_load16(field));
}
#define ACTOR_ID(actor) mine_actor_identifier(actor)
#endif

LOOKUP_RESULT func_8014C118(LOOKUP_CONTROLLER controller, s32 type, s16 id)
{
    u32 i = 0;
    s32 wanted = id;
    Obj8014A2E4 *slot = D_801202A0;
    (void)controller;
    do {
        if (slot->active == type && ACTOR_ID(slot) == wanted)
            return (LOOKUP_RESULT)slot;
        ++i;
        ++slot;
    } while (i < 96);
    return 0;
}
