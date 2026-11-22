#include "steam_knight_hit.h"

extern void func_80186424(SteamKnightPart *part);
extern void func_80132F40(SteamKnightPart *part);
extern void func_80186F60(SteamKnightPart *part);
extern void func_801865D0(SteamKnightPart *part);
extern s32 func_8005C49C(void);
extern void func_80189E34(SteamKnightPart *part, u16 mode);
extern void func_8018A0E4(SteamKnightPart *part);
extern u16 D_801E43BC, D_800B99D8, D_801E4BBC;

#if defined(__mips__) && !defined(__LP64__)
extern void (*D_801964A8[])(SteamKnightPart *);
#define RUN_STATE(part) D_801964A8[(part)->state](part)
#define LINKED(address) ((SteamKnightPart *)(address))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);
extern void bfm_tower_spawn_setup(u32 target, void *actor);
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define LINKED(address) bfm_steam_knight_break_actor(address)
static void run_state(SteamKnightPart *part)
{
    u32 target;
    memcpy(&target, bfm_tower_spawn_pointer(0x801964a8u + 4u * part->state),
           sizeof target);
    bfm_tower_spawn_setup(target, part);
}
#define RUN_STATE(part) run_state(part)
#endif

void func_8018550C(SteamKnightPart *part)
{
    u32 flags;
    u8 status;
    RUN_STATE(part);
    flags = LINKED(part->linked_address)->phase_flags;
    if (flags & 0x8000)
        return;
    func_80186424(part);
    status = ((u8 *)part)[0x74];
    if (status && (part->break_count & 1))
        D_801E43BC |= status;
    if (part->variant == 17 && (part->flags & 0x400))
        func_80132F40(part);
    if ((part->break_count & 2) && (flags & 0x20))
        func_80186F60(part);
    else if ((part->break_count & 1) && (flags & 0x100))
        func_801865D0(part);
    if (flags & 0x10000) {
        if ((D_800B99D8 & 3) == 0) {
            int selection = func_8005C49C();
            if ((selection & 31) == (part->variant & 31))
                func_80189E34(part, D_801E4BBC);
        }
    } else if (flags & 0x4000) {
        if ((u16)(D_800B99D8 % 60u) == part->variant * 2)
            func_8018A0E4(part);
    }
}
