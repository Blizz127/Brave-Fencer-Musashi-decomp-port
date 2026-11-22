#ifndef BFM_MINE_EMITTER_PROJECTION_H
#define BFM_MINE_EMITTER_PROJECTION_H

#include "mine_guest_abi.h"
#include "../main_0012/scene_object_probes.h"

typedef struct { u32 words[8]; } MineEmitterProjectionMatrix;

extern MineEmitterProjectionMatrix D_800AE620;
extern SceneProbeVector D_8019B5AC;
s32 func_8012E32C(void);
void func_8017E910(Obj8014A2E4 *actor);

#if BFM_MINE_DIRECT_GUEST_ADDRESS
void func_8004A0DC(s32 yaw, void *matrix);
void func_8004914C(void *matrix);
s32 func_800491AC(void *matrix);
void func_8004945C(SceneProbeVector *input, SceneProbeVector *output, s32 *flag);
s32 func_8004955C(void *vector, void *screen, void *cue, void *flag);
#else
/* The host resolves only the signed yaw of the selected actor's model. */
int bfm_mine_emitter_model_yaw(u32 model_word, s16 *yaw);
void bfm_mine_emitter_rotate(s32 yaw, MineEmitterProjectionMatrix *matrix);
void bfm_mine_emitter_set_rotation(MineEmitterProjectionMatrix *matrix);
void bfm_mine_emitter_set_translation(MineEmitterProjectionMatrix *matrix);
void bfm_mine_emitter_transform(SceneProbeVector *input, SceneProbeVector *output, s32 *flag);
s32 bfm_mine_emitter_project(SceneProbeVector *vector, s32 *screen, s32 *cue, s32 *flag);
#endif
#endif
