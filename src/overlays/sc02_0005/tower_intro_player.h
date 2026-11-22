#ifndef BFM_TOWER_INTRO_PLAYER_H
#define BFM_TOWER_INTRO_PLAYER_H

#include "psx_types.h"

typedef struct {
    u8 reserved_000[0x204];
    u32 timer;
    u8 reserved_208[12];
    u8 step;
} TowerIntroPlayer;

/* These table callbacks also forward the MIPS result register. */
#if defined(__mips__) && !defined(__LP64__)
#define TOWER_INTRO_RESULT(name) register u32 name __asm__("$2")
#else
#define TOWER_INTRO_RESULT(name) u32 name
#endif

extern s32 D_801270C8;
u32 func_8017FCFC(TowerIntroPlayer *player);
u32 func_8017FF8C(TowerIntroPlayer *player);
u32 func_8017FFF0(TowerIntroPlayer *player);

#endif
