#include "psx_types.h"

typedef struct {
    u16 kind;
    u8 reserved02[4];
    u16 x;
    u8 reserved08[2];
    u16 y;
    u8 reserved0c[2];
    u16 z;
    u8 reserved10[0xfc];
} ActorSlot;

typedef struct {
    u16 x, y, z;
    s16 kind, selector, flags, aim, auxiliary;
    s32 user;
} ActorPlacement;

extern s32 func_8012C890(void *placement, u8 *slot, u8 *parent);

#if defined(__mips__) && !defined(__LP64__)
extern s16 D_80126720;
#define POOL_END() ((ActorSlot *)&D_80126720)
s32 func_8012C658(s16 kind, s16 selector, ActorSlot *parent)
#else
#include "steam_knight_hit.h"
extern void *bfm_tower_actor_slots(void);
extern void *bfm_tower_spawn_pointer(u32 address);
#define POOL_END() ((ActorSlot *)bfm_tower_actor_slots() + 96)
SteamKnightPart *func_8012C658(int kind, u32 selector, int parent_address)
#endif
{
    ActorPlacement placement;
    ActorSlot *slot;
    ActorSlot *end;
    ActorPlacement *request;
    s32 result;
#if !(defined(__mips__) && !defined(__LP64__))
    ActorSlot *parent = parent_address ? (ActorSlot *)bfm_tower_spawn_pointer((u32)parent_address) : 0;
#endif

    if (parent != 0 && parent->kind != 0) {
        placement.x = parent->x;
        placement.y = parent->y;
        placement.z = parent->z;
    } else {
        placement.z = 0;
        placement.y = 0;
        placement.x = 0;
    }
    placement.kind = kind;
    request = &placement;
    end = POOL_END();
    placement.selector = selector;
    placement.flags = 0;
    placement.user = 0;
    placement.auxiliary = 0;
    placement.aim = 0x7fff;
    if (parent == 0) {
        slot = end - 96;
    } else {
        slot = parent + 1;
    }
    for (; slot != end; slot++) {
        if (slot->kind == 0) {
            goto found;
        }
    }
    slot = 0;
found:
    if (slot == 0) {
        result = 0;
    } else {
        result = func_8012C890(request, (u8 *)slot, (u8 *)parent);
    }
#if defined(__mips__) && !defined(__LP64__)
    return result;
#else
    return result ? (SteamKnightPart *)bfm_tower_spawn_pointer((u32)result) : 0;
#endif
}
