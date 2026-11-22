#ifndef BFM_STEAM_KNIGHT_LIMITS_H
#define BFM_STEAM_KNIGHT_LIMITS_H
#include "steam_knight_hit.h"

typedef struct {
    s16 right, left;
} BodyBounds;

typedef struct {
    s16 threshold;
    s16 unused[2];
    s16 value;
} SteamKnightMarker;

#if defined(__mips__) && !defined(__LP64__)
extern u16 D_801E4BBC;
extern s16 D_801E43B4;
extern s32 D_801270C8;
extern s16 D_80195ACE;
extern BodyBounds D_80195AD0[];
extern s16 D_80195AC4[][2];
extern SteamKnightMarker D_80195AF4[];
#define ROUND_INDEX D_801E4BBC
#define COUNT D_801E43B4
#define OBSERVED_VALUE D_801270C8
#define LEFT_EDGE D_80195ACE
#define X_LOWER(i) (D_80195AD0[i].right)
#define X_UPPER(i) (D_80195AD0[i].left)
#define Z_LIMIT(i) (D_80195AC4[i][0])
#define MARKER(i) D_80195AF4[i]
#define POS_X(body) (*(s16 *)((u8 *)(body) + 6))
#define SET_X(body,value) (POS_X(body) = (value))
#else
#include <string.h>
extern void *bfm_tower_spawn_pointer(u32 address);

static inline s16 tower_signed_half(const void *p)
{
    s16 value;
    memcpy(&value, p, sizeof value);
    return value;
}

static inline u16 tower_unsigned_half(const void *p)
{
    u16 value;
    memcpy(&value, p, sizeof value);
    return value;
}

static inline void tower_set_half(void *p, u16 value)
{
    memcpy(p, &value, sizeof value);
}

static inline void tower_set_word(void *p, s32 value)
{
    memcpy(p, &value, sizeof value);
}
#define ROUND_INDEX tower_unsigned_half(bfm_tower_spawn_pointer(0x801e4bbcu))
#define Z_LIMIT(i) tower_signed_half(bfm_tower_spawn_pointer(0x80195ac4u + (i) * 4u))
#define X_UPPER(i) tower_signed_half(bfm_tower_spawn_pointer(0x80195ad2u + (i) * 4u))
#define X_LOWER(i) tower_signed_half(bfm_tower_spawn_pointer(0x80195ad0u + (i) * 4u))
#define LEFT_EDGE tower_signed_half(bfm_tower_spawn_pointer(0x80195aceu))
#define POS_X(body) tower_signed_half((const u8 *)(body) + 6)
#define SET_X(body,value) tower_set_half((u8 *)(body) + 6, (u16)(value))
#endif
#endif
