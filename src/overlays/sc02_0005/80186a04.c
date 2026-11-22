#include "steam_knight_hit.h"

typedef struct {
    s16 x, y, z, id;
} SteamKnightReaction;

extern s32 D_801E43A0;
/* The variant is signed. The room's lookup starts 32768 halfwords from
 * the beginning of the range it can address. */
extern u16 D_80185B4C[];
extern SteamKnightReaction D_801961B8[];
extern SteamKnightReaction D_801961E0[];

void func_80186A04(SteamKnightPart *part)
{
    int variant = part->variant;
    u16 id;
    SteamKnightReaction *list;
    int i, j;

    if (variant >= 14) return;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    /* The historical compiler emits a variable shift with a five-bit count. */
    if ((D_801E43A0 >> (variant + 1)) & 1) return;
#else
    if (((u32)D_801E43A0 >> ((variant + 1) & 31)) & 1) return;
#endif
    id = D_80185B4C[32768 + variant];
    if (id >= 22) {
        list = D_801961E0;
        id -= 22;
    } else {
        list = D_801961B8;
    }

    for (i = 0; i < 4; i++) {
        if (list[i].id == -1) {
            list[i].id = id;
            return;
        }
        if (list[i].id == id) return;
        if (list[i].id > id) {
            for (j = 3; j > i; --j) list[j].id = list[j - 1].id;
            list[i].id = id;
            return;
        }
    }
}
