#include "steam_knight_hit.h"

extern u32 D_801E43CC[];
extern u16 D_801E4BBC;
extern void func_80187280(SteamKnightPart *part, u16 index);
extern void func_80189F64(void);
extern void func_801876B0(SteamKnightPart *part);
extern void func_80187664(SteamKnightPart *part);
extern void func_8012AD44(u8 *actor, u16 state);

#if defined(__mips__) && !defined(__LP64__)
#define BREAK_ACTOR(address) ((SteamKnightPart *)(address))
#else
/* Resolve the serialized actor address in the current room. */
extern SteamKnightPart *bfm_steam_knight_break_actor(u32 address);
#define BREAK_ACTOR(address) bfm_steam_knight_break_actor(address)
#endif

void func_80185060(SteamKnightPart *part, int mode)
{
    int special;

    func_80187280(part, part->variant + 1);
    part->reaction = 0;
    part->field_dc = 1;
    if (!part->break_cause) part->break_cause = mode;

    special = 0;
    switch (part->variant) {
    case 3:
        BREAK_ACTOR(part->linked_address)->phase_flags |= 0x2000;
        func_80189F64();
        special = 1;
        break;
    case 8:
    case 10:
        BREAK_ACTOR(D_801E43CC[part->variant + 1])->flags = 0x8800;
        break;
    case 9:
        BREAK_ACTOR(part->body_address)->phase_flags |= 0x20000;
        break;
    case 11:
        BREAK_ACTOR(part->body_address)->phase_flags |= 0x40000;
        break;
    }
    if (special) {
        part->break_delay = 24;
        if (mode) func_801876B0(part);
    } else {
        part->break_delay = 16;
        if (mode) func_80187664(part);
        if (!D_801E4BBC) ++BREAK_ACTOR(part->linked_address)->break_count;
    }
    part->field_ae = -1;
    func_8012AD44((u8 *)part, 2);
}
