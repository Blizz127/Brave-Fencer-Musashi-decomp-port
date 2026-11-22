#include "steam_knight_hit.h"

extern u16 D_80185B4C[];
extern u32 D_801E4418[], D_801E4474[], D_801BD9D4[];
extern s32 D_801E43A0;
extern u8 D_80195CA4[][16];

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
#define RECORD_WORD(index) ((u32)&D_80195CA4[index])
#else
#define RECORD_WORD(index) (0x80195CA4u + 16u * (index))
#endif

void func_80187280(SteamKnightPart *part, u16 key)
{
    u16 entry = key;
    u16 slot;
    u32 *bank;
    if (key < 14) {
        if (key != 0) {
            /* Key one addresses the reaction table entry for variant zero. */
            slot = D_80185B4C[32767 + key];
            bank = D_801E4418;
            if (slot >= 22) {
                bank = D_801E4474;
                slot -= 22;
            }
            --entry;
            if (slot == 5) {
                bank[5] = 0x01000000;
            } else {
                bank[slot] = D_801BD9D4[entry];
            }
            part->display_record = RECORD_WORD(entry) | 0x40000000u;
        }
        D_801E43A0 |= 1u << key;
    }
}
