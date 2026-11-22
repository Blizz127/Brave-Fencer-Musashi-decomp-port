/* Check the actor pool for side contacts and surfaces under the actor. */
#include "forest_actor_contact.h"

extern u16 D_800B99DA;
extern u32 D_801150D8;

#if FOREST_DIRECT_ACTOR
extern s16 D_801152AA, D_80126724;
#define NORMAL_Y D_801152AA
#define CENTER_Z D_80126724
#define SLOT_MATCHES(slot, reference) ((slot) == *(ForestContactSlot **)(reference))
#define SLOT_FLAGS(slot) ((slot)->status.word)
#else
#define NORMAL_Y ((s16)D_801152A8.y)
#define CENTER_Z ((s16)D_80126720.z)
#define SLOT_MATCHES(slot, reference) (forest_actor_slot_address(slot) == FOREST_ACTOR_WORD(reference))
#define SLOT_FLAGS(slot) ((slot)->flags)
#endif
#define CENTER_X ((s16)D_80126720.x)

s32 func_8014EA4C(u8 *actor, FloorPoint *probe, FloorPoint *end, s32 pass)
{
#if FOREST_DIRECT_ACTOR
    FloorPoint hit, push, saved;
#endif
    ForestContactSlot *slot;
    s32 result = 0;
    s32 dx, dz;
    (void)pass;
#if FOREST_DIRECT_ACTOR
    /* The original frame contains two unused points and a copy of the endpoint. */
    (void)hit;
    (void)push;
    saved = *end;
#endif
    dx = 8;
    dz = 8;
    switch ((D_800B99DA & 0x60) >> 5) {
    case 0:
    case 1:
        dx = 7;
        break;
    case 2:
        dz = 7;
        break;
    }
    for (slot = D_801202A0; slot < &D_801202A0[96]; ++slot) {
        FloorContactRecord *record;
        s32 collision;
        if (!slot->id || !slot->collision) continue;
        if (SLOT_MATCHES(slot, actor + 0x178)) continue;
        if (SLOT_MATCHES(slot, actor + 0x174)) continue;
        if (!(SLOT_FLAGS(slot) & 0x1800)) continue;
        D_801150D8 = 0;
        collision = (s32)slot->collision;
#if FOREST_DIRECT_ACTOR
        record = (FloorContactRecord *)slot->record;
#else
        {
            u32 address = slot->record;
            if (!address) bfm_forest_model_invalid_handle(address);
            record = FOREST_ACTOR_REF(FloorContactRecord, address);
            if (!record) bfm_forest_model_invalid_handle(address);
        }
#endif
        if (!func_80135A4C(record, collision, probe, end)) continue;
        if (D_801150D8 & 1) {
            if (!(FOREST_SLOT_FLAGS(slot) & 0x10)) {
                if (CENTER_X - (s16)probe->x >= 0)
                    FOREST_ACTOR_SET_HALF(actor + 6, (s16)end->x - dx);
                else
                    FOREST_ACTOR_SET_HALF(actor + 6, (s16)end->x + dx);
                if (CENTER_Z - (s16)probe->z >= 0)
                    FOREST_ACTOR_SET_HALF(actor + 0xe, (s16)end->z - dz);
                else
                    FOREST_ACTOR_SET_HALF(actor + 0xe, (s16)end->z + dz);
            }
            result |= 2;
        } else {
            FOREST_ACTOR_SET_HALF(actor + 6, end->x);
            FOREST_ACTOR_SET_HALF(actor + 0xa, end->y + 16);
            FOREST_ACTOR_SET_HALF(actor + 0xe, end->z);
            result |= 1;
            if ((s32)slot->collision < 0) {
                if ((s16)(NORMAL_Y < 0 ? -NORMAL_Y : NORMAL_Y) > 0x800) {
                    if (CENTER_X - (s16)end->x >= 0)
                        FOREST_ACTOR_SET_HALF(actor + 6, (s16)end->x - dx);
                    else
                        FOREST_ACTOR_SET_HALF(actor + 6, (s16)end->x + dx);
                    if (CENTER_Z - (s16)end->z >= 0)
                        FOREST_ACTOR_SET_HALF(actor + 0xe, (s16)end->z - dz);
                    else
                        FOREST_ACTOR_SET_HALF(actor + 0xe, (s16)end->z + dz);
                }
            }
        }
    }
    return result;
}
