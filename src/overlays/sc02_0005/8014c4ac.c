#include "steam_knight_hit.h"

#if !(defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2)
#include <string.h>
#endif

extern SteamKnightPart *D_80126CD0;
extern s16 D_8012693A;
extern s16 D_801152B8;

SteamKnightPart *func_8014C4AC(SteamKnightPart *part, int kind, int amount,
                               const void *position, u16 parameter)
{
    if (part && part->active) {
        if (kind != 9 && kind != 17 && kind != 41 && kind != 10 && D_80126CD0 == part) {
            return 0;
        }
        part->hit_kind = kind;
        part->contact_parameter = parameter;
        part->damage = amount;
        part->flags |= 1;
        /* The game reads both position words before writing either one. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
        part->hit_position = *(const SteamKnightHitPosition *)position;
#else
        memmove(part->hit_position.bytes, position, sizeof part->hit_position.bytes);
#endif
        part->contact_frame = D_8012693A;
        part->contact_tag = D_801152B8;
        return part;
    }
    return 0;
}
