/* Rotate the player to the requested heading before the phase-14 transition. */
#include "tower_intro_player.h"

extern s32 func_80012B04(s16 from, s16 to, s16 divisor);
extern s32 func_801399F0(void *request);
extern void func_80147060(u8 *player);
extern s32 func_80139914(void *request);
extern u32 func_8017E26C(void);

#if defined(__mips__) && !defined(__LP64__)
#define PLAYER_PTR(player, offset) (*(u8 **)((u8 *)(player) + (offset)))
#define HEADING(model) (*(u16 *)((model) + 0x12))
#define SET_HEADING(model, value) (HEADING(model) = (value))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
static u8 *player_pointer(TowerIntroPlayer *player, unsigned offset)
{
    u32 address;
    memcpy(&address, (u8 *)player + offset, sizeof address);
    return address ? bfm_tower_spawn_pointer(address) : 0;
}
static u16 heading(u8 *model)
{
    u16 value;
    memcpy(&value, model + 0x12, sizeof value);
    return value;
}
static void set_heading(u8 *model, u16 value)
{
    memcpy(model + 0x12, &value, sizeof value);
}
#define PLAYER_PTR(player, offset) player_pointer((player), (offset))
#define HEADING(model) heading(model)
#define SET_HEADING(model, value) set_heading((model), (value))
#endif

u32 func_8017FCFC(TowerIntroPlayer *player)
{
    u8 *model;
    s32 turn = func_80012B04((s16)HEADING(PLAYER_PTR(player, 0x20)), 0x800, 4);
    if (!(s16)turn && func_801399F0(PLAYER_PTR(player, 0x198))) {
        func_80147060((u8 *)player);
        func_80139914(PLAYER_PTR(player, 0x198));
        func_8017E26C();
        D_801270C8 = 14;
        ++player->step;
    }
    model = PLAYER_PTR(player, 0x20);
    {
        u32 value = (HEADING(model) + (u32)turn) & 0xFFF;
        SET_HEADING(model, value);
        return value;
    }
}
