#include "steam_knight_hit.h"

extern u8 D_801E44E0[];
extern u32 D_801E44EC;
extern void func_8013240C(void *effect);
extern void func_80183FCC(SteamKnightPart *core);
extern void func_8012AD44(u8 *actor, u16 state);

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80196338[];
extern void func_80132784(SteamKnightPart *actor, SteamKnightPart *parent, int mode);
#define ATTACH(actor, address) func_80132784((actor), (SteamKnightPart *)(address), 21)
#define OPEN_RECORD ((u32)D_80196338 | 0x40000000u)
#define BODY(address) ((SteamKnightPart *)(address))
#define ENABLE_MODEL(actor) (*(u16 *)(*(u8 **)((u8 *)(actor) + 0x20) + 0x2c) |= 1)
#else
#include <string.h>
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
extern u8 *bfm_steam_knight_core_model(u32 address);
/* The attachment service receives the current serialized parent handle. */
extern void bfm_steam_knight_core_attach(SteamKnightPart *actor, u32 parent_address, int mode);
#define ATTACH(actor, address) bfm_steam_knight_core_attach((actor), (address), 21)
#define OPEN_RECORD 0xc0196338u
#define BODY(address) bfm_steam_knight_break_actor(address)
static void enable_model(const SteamKnightPart *actor)
{
    u32 model;
    u16 flags;
    u8 *field;
    memcpy(&model, (const u8 *)actor + 0x20, sizeof model);
    field = bfm_steam_knight_core_model(model) + 0x2c;
    memcpy(&flags, field, sizeof flags);
    flags |= 1;
    memcpy(field, &flags, sizeof flags);
}
#define ENABLE_MODEL(actor) enable_model(actor)
#endif

void func_8018850C(SteamKnightPart *core)
{
    ENABLE_MODEL(core);
    func_8013240C(D_801E44E0);
    if (D_801E44EC & 0x4000) {
        BODY(core->body_address)->phase_flags |= 2;
        func_80183FCC(core);
        core->reaction = 0;
        core->hit_kind = 0;
        func_8012AD44((u8 *)core, 2);
        core->display_record = OPEN_RECORD;
        ATTACH(core, core->body_address);
    }
}
