#ifndef BFM_STEAM_KNIGHT_ACTION_SEARCH_H
#define BFM_STEAM_KNIGHT_ACTION_SEARCH_H

#include "steam_knight_hit.h"

s32 func_801860F4(SteamKnightPart *body, s16 action);
s16 func_80186144(SteamKnightPart *body, int action);
s32 func_8018AC48(SteamKnightPart *body);
s32 func_8018AD20(SteamKnightPart *body);

#if defined(__mips__) && !defined(__LP64__)
extern SteamKnightPart *D_801E4A58[12];
#define ACTION_SLOT(index) D_801E4A58[index]
#define SLOT_BUSY(part) (*(const s16 *)&(part)->field_fc)
#define SLOT_Z(part) (part)->field_0e
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
static inline u16 bfm_action_half(const void *object, unsigned offset)
{
    u16 result;
    memcpy(&result, (const u8 *)object + offset, sizeof result);
    return result;
}

static inline void *bfm_action_slot(unsigned index)
{
    u32 address;
    memcpy(&address, bfm_tower_spawn_pointer(0x801e4a58u + index * 4u), sizeof address);
    return address ? bfm_tower_spawn_pointer(address) : 0;
}
#define ACTION_SLOT(index) bfm_action_slot(index)
#define SLOT_BUSY(part) bfm_action_half(part, 0xfc)
#define SLOT_Z(part) bfm_action_half(part, 0xe)
#endif

#endif
