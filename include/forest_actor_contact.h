#ifndef MUSASHI_FOREST_ACTOR_CONTACT_H
#define MUSASHI_FOREST_ACTOR_CONTACT_H

#include "forest_floor.h"

typedef struct {
    u16 id;
    u8 pad02[4];
    u16 field06;
    u8 pad08[2];
    u16 field0a;
    u8 pad0c[2];
    u16 field0e;
    u8 pad10[16];
    u32 record;
    u8 pad24[52];
    u32 collision;
    u16 flags;
    u8 pad5e[174];
} ForestContactSlot;

extern ForestContactSlot D_801202A0[96];

#ifdef __mips__
#define FOREST_ACTOR_REF(type, address) ((type *)(address))
#define FOREST_ACTOR_STORE(destination, slot) (*(ForestContactSlot **)(destination) = (slot))
#else
#include <string.h>
/* Resolve a game address in the current owner's RAM or scratchpad. */
extern void *bfm_forest_contact_pointer(u32 address, u32 width);
#define FOREST_ACTOR_REF(type, address) ((type *)bfm_forest_contact_pointer((address), sizeof(type)))
static inline void forest_actor_store(u8 *destination, const ForestContactSlot *slot)
{
    u32 address = 0x801202a0u + (u32)(slot - D_801202A0) * sizeof(*slot);
    memcpy(destination, &address, sizeof address);
}
#define FOREST_ACTOR_STORE(destination, slot) forest_actor_store((destination), (slot))
#endif

s32 func_8014E5B4(u8 *object, u32 start, u32 end);
s32 func_8014E83C(u8 *object, u32 start, u32 end);
#endif
