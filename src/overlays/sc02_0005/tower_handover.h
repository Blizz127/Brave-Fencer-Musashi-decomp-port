#ifndef BFM_TOWER_HANDOVER_H
#define BFM_TOWER_HANDOVER_H
#include "tower_intro_player.h"
#include "tower_request.h"
#include "../sc02_0039/mine_frame_state.h"

extern u8 D_800B9AAC[];
extern u16 D_80078EB4;
u32 func_80175414(void);
u32 func_8014C6AC(void);
s32 func_8016F0E4(u8 *player);
void func_8017196C(u8 *player, u8 step);
u32 func_801511A8(u8 *player);
s32 func_80148634(void *player);

#if defined(__mips__) && !defined(__LP64__)
extern s32 func_80165770(u8 *player);
#define HANDOVER_LAST(player) ((u32)func_80165770((u8 *)(player)))
extern u8 D_800B9A17;
#define HANDOVER_LIVE_CONTROL D_800B9A17
extern volatile u8 D_8011F7A9, D_8011F7AD, D_8011F7AE;
extern u8 D_80126D17;
#define HANDOVER_MODE_BYTE1 D_8011F7A9
#define HANDOVER_MODE_BYTE5 D_8011F7AD
#define HANDOVER_MODE_BYTE6 D_8011F7AE
#define HANDOVER_FLAG D_80126D17
#define HANDOVER_HALF(p, o) (*(u16 *)((u8 *)(p) + (o)))
#define HANDOVER_SET_HALF(p, o, v) (HANDOVER_HALF((p), (o)) = (v))
#define HANDOVER_ZERO_WORD(p, o) (*(u32 *)((u8 *)(p) + (o)) = 0)
#else
#include <string.h>
#define HANDOVER_MODE_BYTE1 D_8011F7A8.reserved[0]
#define HANDOVER_MODE_BYTE5 D_8011F7A8.reserved[4]
#define HANDOVER_MODE_BYTE6 D_8011F7A8.reserved[5]
extern void *bfm_tower_spawn_pointer(u32 address);
/* SC02/0005 forwards the player at this member-specific ability check. */
s32 bfm_tower_handover_ability_service(u8 *player);
#define HANDOVER_LAST(player) ((u32)bfm_tower_handover_ability_service((u8 *)(player)))
#define HANDOVER_LIVE_CONTROL (*(u8 *)bfm_tower_spawn_pointer(0x800b9a17u))
#define HANDOVER_FLAG (*(u8 *)bfm_tower_spawn_pointer(0x80126d17u))
static inline u16 handover_half(const void *player, unsigned offset)
{
    u16 value;
    memcpy(&value, (const u8 *)player + offset, sizeof value);
    return value;
}
static inline void handover_set_half(void *player, unsigned offset, u16 value)
{
    memcpy((u8 *)player + offset, &value, sizeof value);
}
static inline void handover_zero_word(void *player, unsigned offset)
{
    u32 zero = 0;
    memcpy((u8 *)player + offset, &zero, sizeof zero);
}
#define HANDOVER_HALF(p, o) handover_half((p), (o))
#define HANDOVER_SET_HALF(p, o, v) handover_set_half((p), (o), (v))
#define HANDOVER_ZERO_WORD(p, o) handover_zero_word((p), (o))
#endif
#endif
