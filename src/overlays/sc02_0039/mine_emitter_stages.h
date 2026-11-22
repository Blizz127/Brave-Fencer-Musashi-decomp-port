#ifndef BFM_MINE_EMITTER_STAGES_H
#define BFM_MINE_EMITTER_STAGES_H

#include "psx_types.h"

typedef struct {
    u16 kind;
    u16 stage;
} MineActorStage;

typedef void (*MineStageAction)(MineActorStage *actor);

extern MineStageAction D_8019B5A4[2];
extern MineStageAction D_8019B634[2];

void func_8017E758(MineActorStage *actor);
void func_8017EE9C(MineActorStage *actor);
#endif
