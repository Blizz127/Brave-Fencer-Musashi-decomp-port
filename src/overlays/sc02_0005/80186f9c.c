/* Create the Steam Knight effect attached to this body or its linked actor. */
#include "steam_knight_hit.h"

extern s32 D_801E43A0;

#if defined(__mips__) && !defined(__LP64__)
#define PARENT_ADDRESS(body) ((s32)(body))
#else
extern u32 bfm_steam_knight_effect_owner_address(const SteamKnightPart *body);
#define PARENT_ADDRESS(body) ((s32)bfm_steam_knight_effect_owner_address(body))
#endif

void func_80186F9C(SteamKnightPart *body, int joint)
{
    u16 index = joint;
    int selector;
    SteamKnightPart *effect;

    if (index < 32) {
        if (index < 22) {
            func_8012C658(81, index, PARENT_ADDRESS(body));
        } else {
            index -= 22;
            selector = 0x100;
            if (index == 0) {
                selector = (D_801E43A0 & 2) ? 0x300 : 0x200;
            }
            if (index == 3) {
                selector = (D_801E43A0 & 8) ? 0x500 : 0x400;
            }
            effect = func_8012C658(81, selector | index, 0);
            if (effect)
                effect->body_address = body->linked_address;
        }
    }
}
