#include "mine_emitter_scan.h"

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#define SOUND(command, argument) func_8002D4C8((command), (argument))
#define ABS_NEGATIVE(value) (-(value))
#define HALF_DISTANCE(value) ((value) >> 1)
#define SIGNED_HALF(actor, offset) (*(s16 *)((u8 *)(actor) + (offset)))
#define UNSIGNED_HALF(actor, offset) (*(u16 *)((u8 *)(actor) + (offset)))
#define SIGNED_WORD(actor, offset) (*(s32 *)((u8 *)(actor) + (offset)))
#define STORE_HALF(actor, offset, value) (UNSIGNED_HALF(actor, offset) = (value))
#else
#include "mine_actor_fields.h"
#define SOUND(command, argument) bfm_mine_emitter_audio((command), (argument))
/* Preserve the signed halfword wrap at -32768 and the arithmetic shift. */
#define ABS_NEGATIVE(value) mine_actor_signed16((u16)(-(value)))
#define HALF_DISTANCE(value) ((value) >= 0 ? (value) / 2 : -((1 - (value)) / 2))

static s32 emitter_signed_word(const u8 *field)
{
    u32 bits = mine_actor_load32(field);
    return (bits & 0x80000000u) ? -1 - (s32)(~bits) : (s32)bits;
}

#define UNSIGNED_HALF(actor, offset) mine_actor_load16((const u8 *)(actor) + (offset))
#define SIGNED_HALF(actor, offset) mine_actor_signed16(UNSIGNED_HALF(actor, offset))
#define SIGNED_WORD(actor, offset) emitter_signed_word((const u8 *)(actor) + (offset))
#define STORE_HALF(actor, offset, value) mine_actor_store16((u8 *)(actor) + (offset), (value))
#endif

void func_8017E794(MineEmitterActor *actor)
{
    s16 nearest = 32767;
    s16 lateral = 32767;
    Obj8014A2E4 *selected = 0;
    Obj8014A2E4 *slot = D_801202A0;
    s32 i;
    u16 tick;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
    if (!actor)
        return;
#endif
    for (i = 0; i < 96; ++i, ++slot) {
        if (slot->active == 0x1a8) {
            s16 distance;
            func_8017E910(slot);
            distance = SIGNED_HALF(slot, 0x100);
            if (distance < 0)
                distance = ABS_NEGATIVE(distance);
            if (nearest > distance) {
                lateral = SIGNED_HALF(slot, 0x100);
                nearest = distance;
                selected = slot;
            }
        }
    }
    tick = UNSIGNED_HALF(actor, 0x84) + 1;
    STORE_HALF(actor, 0x84, tick);
    if (nearest < 160 && SIGNED_WORD(selected, 0xe0) < 336) {
        if ((tick & 3) == 0) {
            s32 pan = (lateral + 160) / 20;
            u16 gain = 127 - HALF_DISTANCE(nearest);
            u32 encoded = (u32)pan << 8 | 0x3000u;
            SOUND(0x6da, gain | encoded);
            STORE_HALF(actor, 0x102, 1);
        }
    } else if (SIGNED_HALF(actor, 0x102) == 1) {
        SOUND(4, 0x6da);
        STORE_HALF(actor, 0x102, 0);
    }
}
