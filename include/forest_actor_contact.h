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
    /* The game reads the status at +0x5c as both a word and a halfword. */
#if defined(__mips__) && !defined(__LP64__)
    union { u32 word; u16 low; } status;
    u8 pad60[172];
#else
    u16 flags;
    u8 pad5e[174];
#endif
} ForestContactSlot;

extern ForestContactSlot D_801202A0[96];

#if defined(__mips__) && !defined(__LP64__)
#define FOREST_DIRECT_ACTOR 1
#define FOREST_SLOT_FLAGS(slot) ((slot)->status.low)
#define FOREST_ACTOR_REF(type, address) ((type *)(address))
#define FOREST_ACTOR_STORE(destination, slot) (*(ForestContactSlot **)(destination) = (slot))
#define FOREST_ACTOR_HALF(p) (*(u16 *)(p))
#define FOREST_ACTOR_WORD(p) (*(u32 *)(p))
#define FOREST_ACTOR_SET_HALF(p, value) (*(u16 *)(p) = (value))
#else
#define FOREST_DIRECT_ACTOR 0
#define FOREST_SLOT_FLAGS(slot) ((slot)->flags)
#include <string.h>
/* Resolve a game address in the current owner's RAM or scratchpad. */
extern void *bfm_forest_contact_pointer(u32 address, u32 width);
#define FOREST_ACTOR_REF(type, address) ((type *)bfm_forest_contact_pointer((address), sizeof(type)))
static inline u32 forest_actor_slot_address(const ForestContactSlot *slot)
{
    return 0x801202a0u + (u32)(slot - D_801202A0) * sizeof(*slot);
}
static inline void forest_actor_store(u8 *destination, const ForestContactSlot *slot)
{
    u32 address = forest_actor_slot_address(slot);
    memcpy(destination, &address, sizeof address);
}
#define FOREST_ACTOR_STORE(destination, slot) forest_actor_store((destination), (slot))
static inline u16 forest_actor_half(const u8 *p)
{
    return (u16)(p[0] | ((u16)p[1] << 8));
}
static inline u32 forest_actor_word(const u8 *p)
{
    return forest_actor_half(p) | ((u32)forest_actor_half(p + 2) << 16);
}
static inline void forest_actor_set_half(u8 *p, u16 value)
{
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
}
#define FOREST_ACTOR_HALF(p) forest_actor_half(p)
#define FOREST_ACTOR_WORD(p) forest_actor_word(p)
#define FOREST_ACTOR_SET_HALF(p, value) forest_actor_set_half((p), (u16)(value))
#endif

s32 func_8014E5B4(u8 *object, u32 start, u32 end);
s32 func_8014E83C(u8 *object, u32 start, u32 end);
s32 func_8014E98C(u8 *actor);
s32 func_8014EA4C(u8 *actor, FloorPoint *probe, FloorPoint *end, s32 pass);
#endif
