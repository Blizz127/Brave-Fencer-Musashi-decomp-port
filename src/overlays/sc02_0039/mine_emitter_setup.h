#ifndef BFM_MINE_EMITTER_SETUP_H
#define BFM_MINE_EMITTER_SETUP_H

#include "psx_types.h"

typedef struct {
    u8 before_timer[0x1c];
    s32 timer;
    u32 model;
    u8 before_delay[0xfc - 0x24];
    s16 initial_delay;
    s16 index;
    u16 reserved100;
    u16 variant;
} MineEmitterSetup;

/* Matches the stage-advance service's view of the actor. */
typedef struct {
    u8 pad0[2];
    u16 cnt;
    u8 pad4[0x30];
    u16 clr;
} MineEmitterStateActor;

#if defined(__mips__) && !defined(__LP64__)
typedef struct {
    s16 unk0, unk2;
    u8 pad4[0x20];
    s32 unk24;
} MineEmitterChildModel;

typedef void *MineEmitterModel;
MineEmitterModel func_8012C1B8(void);
void func_8001C2C4(MineEmitterChildModel *model);
#else
/* The actor stores the game's 32-bit model address. */
typedef u32 MineEmitterModel;
MineEmitterModel bfm_mine_emitter_acquire_model(void);
void bfm_mine_emitter_bind_model(MineEmitterModel model);
#endif
s32 func_8012CAE4(u8 *actor);
void func_8012AD50(MineEmitterStateActor *actor);
void func_8017F050(MineEmitterSetup *actor);

#endif
