#include "steam_knight_core.h"

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_80196208[];
#define LINKED(address) ((SteamKnightPart *)(address))
#define SET_MODEL_RECORD(actor) (*(u32 *)(*(u8 **)((u8 *)(actor) + 0x20) + 0x80) = (u32)D_80196208)
#else
#include <string.h>
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
extern u8 *bfm_steam_knight_core_model(u32 address);
#define LINKED(address) bfm_steam_knight_break_actor(address)
static void set_model_record(const SteamKnightPart *actor)
{
    u32 model;
    u32 record = 0x80196208u;
    memcpy(&model, (const u8 *)actor + 0x20, sizeof model);
    memcpy(bfm_steam_knight_core_model(model) + 0x80, &record, sizeof record);
}
#define SET_MODEL_RECORD(actor) set_model_record(actor)
#endif

void func_801869E0(SteamKnightPart *body)
{
    SET_MODEL_RECORD(body);
    SET_MODEL_RECORD(LINKED(body->linked_address));
}
