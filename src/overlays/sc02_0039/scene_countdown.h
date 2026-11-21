#ifndef BFM_SC02_0039_SCENE_COUNTDOWN_H
#define BFM_SC02_0039_SCENE_COUNTDOWN_H
#include "psx_types.h"

typedef struct {
    u8 reserved0[0x15];
    u8 phase;
    u8 reserved16[0x12];
    u32 timer;
} SceneCountdown;

typedef s32 (*ScenePhaseHandler)(SceneCountdown *);

s32 func_8017CD9C(SceneCountdown *state);
s32 func_8017CDD8(SceneCountdown *state);
s32 func_8017CE1C(SceneCountdown *state);
void func_8017CFCC(void);
s32 func_8017CE40(SceneCountdown *state);
s32 func_8017CEFC(SceneCountdown *state);
s32 func_8017CF1C(SceneCountdown *state);
s32 func_8017CF58(SceneCountdown *state);
void func_8017CFF4(void);

#endif
