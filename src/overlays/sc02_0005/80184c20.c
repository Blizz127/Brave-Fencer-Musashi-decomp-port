/* Reset the auxiliary actor and attach it to its body's current linked part. */
#include "steam_knight_hit.h"

extern void func_8012AD44(u8 *actor, u16 state);
extern void func_8012B200(void *actor);

#if defined(__mips__) && !defined(__LP64__)
extern u8 D_801963FC[];
extern void func_80132784(SteamKnightPart *actor, SteamKnightPart *parent, int index);
#define DISPLAY_RECORD ((u32)D_801963FC)
#define MARK_LINK(address) (((SteamKnightPart *)(address))->field_fc = 1)
#define PARENT(address) (((SteamKnightPart *)(address))->linked_address)
#define ATTACH(actor, address) func_80132784((actor), (SteamKnightPart *)(address), 6)
#define CLEAR_POSITION(actor) (((s32 *)(actor))[1] = ((s32 *)(actor))[2] = ((s32 *)(actor))[3] = 0)
#else
#include <stddef.h>
#include <string.h>

extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_steam_knight_core_attach(SteamKnightPart *actor, u32 parent_address, int index);
#define DISPLAY_RECORD 0x801963fcu
#define ATTACH(actor, address) bfm_steam_knight_core_attach((actor), (address), 6)
#define CLEAR_POSITION(actor) memset((u8 *)(actor) + 4, 0, 3 * sizeof(s32))

static void mark_link(u32 address)
{
    u8 *child = bfm_tower_spawn_pointer(address);
    u16 marker = 1;
    memcpy(child + offsetof(SteamKnightPart, field_fc), &marker, sizeof marker);
}

static u32 parent_link(u32 address)
{
    const u8 *body = bfm_tower_spawn_pointer(address);
    u32 linked;
    memcpy(&linked, body + offsetof(SteamKnightPart, linked_address), sizeof linked);
    return linked;
}
#define MARK_LINK(address) mark_link(address)
#define PARENT(address) parent_link(address)
#endif

void func_80184C20(SteamKnightPart *actor)
{
    u32 child = actor->linked_address;
    u32 body;
    if (child) {
        MARK_LINK(child);
        actor->linked_address = 0;
    }
    actor->display_record = DISPLAY_RECORD;
    actor->flags = 0x8c00;
    func_8012AD44((u8 *)actor, 1);
    func_8012B200(actor);
    body = actor->body_address;
    CLEAR_POSITION(actor);
    ATTACH(actor, PARENT(body));
}
