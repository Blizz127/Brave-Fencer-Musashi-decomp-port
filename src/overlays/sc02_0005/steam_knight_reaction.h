#ifndef BFM_STEAM_KNIGHT_REACTION_H
#define BFM_STEAM_KNIGHT_REACTION_H

#include "psx_types.h"

typedef struct {
    s16 x, y, z, id;
} SteamKnightReaction;

/* The variant is signed. The room's lookup starts 32768 halfwords from
 * the beginning of the range it can address. */
extern u16 D_80185B4C[];
/* Both lists have four visible IDs. Removal reads the following record's ID. */
extern SteamKnightReaction D_801961B8[];
extern SteamKnightReaction D_801961E0[];

#endif
