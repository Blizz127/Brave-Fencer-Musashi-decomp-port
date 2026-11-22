/* Release active Steam Knight effects attached to the body. */
#include "steam_knight_hit.h"

extern void func_8012B23C(s32 *effect);
extern void func_8012AD44(u8 *effect, u16 state);

#if defined(__mips__) && !defined(__LP64__)
extern SteamKnightPart *D_801E4A58[12];
extern void func_80132784(SteamKnightPart *effect, SteamKnightPart *owner, int joint);
#define ATTACH(effect, owner, joint) func_80132784((effect), (SteamKnightPart *)(owner), (joint))
#define ACTOR(value) ((SteamKnightPart *)(value))
#define MODEL(effect) ((s32 *)(effect)->model_address)
#define POSITION_X(effect,value) (*(s16 *)((u8 *)(effect) + 6) = (value))
#else
#include <string.h>
extern u32 D_801E4A58[12];
extern void bfm_steam_knight_core_attach(SteamKnightPart *effect, u32 owner, int joint);
#define ATTACH bfm_steam_knight_core_attach
extern void *bfm_tower_spawn_pointer(u32 value);
#define ACTOR(value) ((value) ? bfm_tower_spawn_pointer(value) : (SteamKnightPart *)0)
static s32 model_word(SteamKnightPart *effect, unsigned index)
{
    s32 result;
    const u8 *model = bfm_tower_spawn_pointer(effect->model_address);
    memcpy(&result, model + 4 * index, sizeof result);
    return result;
}
static void set_position_x(SteamKnightPart *effect, s16 x)
{
    memcpy((u8 *)effect + 6, &x, sizeof x);
}
#define POSITION_X(effect,value) set_position_x((effect), (s16)(value))
#endif
#if defined(__mips__) && !defined(__LP64__)
#define MODEL_WORD(effect,index) MODEL(effect)[index]
#else
#define MODEL_WORD(effect,index) model_word((effect),(index))
#endif

void func_8018AAAC(void)
{
    int i;
    SteamKnightPart *effect;

    for (i = 0; i < 12; i++) {
        effect = ACTOR(D_801E4A58[i]);
        if (effect && effect->state == 2) {
            ATTACH(effect, effect->body_address, effect->break_count);
            POSITION_X(effect, MODEL_WORD(effect, 18));
            effect->position_y = MODEL_WORD(effect, 19);
            effect->field_0e = MODEL_WORD(effect, 20);
            func_8012B23C((s32 *)effect);
            effect->motion_x = 0x100000;
            effect->motion_y = -0x200000;
            effect->motion_z = -0x40000;
            effect->timer = 32;
            func_8012AD44((u8 *)effect, 5);
        }
    }
}
