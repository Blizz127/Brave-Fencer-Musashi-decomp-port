/* Resolve the first enabled actor contact and copy its result fields. */
#include "forest_actor_contact.h"

s32 func_8014E83C(u8 *object, u32 start, u32 end)
{
    ForestContactSlot *slot;

    for (slot = D_801202A0; slot < &D_801202A0[96]; slot++) {
        if (slot->id != 0 && (FOREST_SLOT_FLAGS(slot) & 0x20) != 0 && slot->collision != 0
            && func_80135888(FOREST_ACTOR_REF(FloorContactRecord, slot->record), slot->collision,
                             FOREST_ACTOR_REF(FloorPoint, start), FOREST_ACTOR_REF(FloorPoint, end)) != 0) {
            FOREST_ACTOR_STORE(object + 0x180, slot);
            *(u16 *)(object + 6) = slot->field06;
            *(u16 *)(object + 0xa) = slot->field0a;
            *(u16 *)(object + 0xe) = slot->field0e;
            return 1;
        }
    }
    return 0;
}
