#include "steam_knight_hit.h"

#if defined(__mips__) && !defined(__LP64__)
#include "steam_knight_reaction.h"
#define BODY_RECORD ((u32)D_801961B8)
#define ATTACHMENT_RECORD ((u32)D_801961E0)
#define RESTORE_RECORD(body, record) (*(u32 *)((body)->model_address + 0x80) = (u32)(record))
#define ATTACHED(body) ((SteamKnightPart *)(body)->linked_address)
#else
#include <string.h>
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
extern u8 *bfm_steam_knight_core_model(u32 address);

static void restore_record(const SteamKnightPart *actor, u32 record)
{
    u8 *model = bfm_steam_knight_core_model(actor->model_address);
    memcpy(model + 0x80, &record, sizeof record);
}

#define RESTORE_RECORD(body, record) restore_record(body, record)
#define ATTACHED(body) bfm_steam_knight_break_actor((body)->linked_address)
#define BODY_RECORD 0x801961B8u
#define ATTACHMENT_RECORD 0x801961E0u
#endif

void func_80186ADC(SteamKnightPart *body)
{
    RESTORE_RECORD(body, BODY_RECORD);
    RESTORE_RECORD(ATTACHED(body), ATTACHMENT_RECORD);
}
