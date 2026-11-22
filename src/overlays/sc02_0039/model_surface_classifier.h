#ifndef BFM_MODEL_SURFACE_CLASSIFIER_H
#define BFM_MODEL_SURFACE_CLASSIFIER_H

#include "model_surface_probe.h"

s32 func_80135480(SceneSurfaceRecord *model, u32 flags, SceneProbeVector *near_point,
                  SceneProbeVector *far_point);

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
void bfm_surface_zero_scale(unsigned axis);
#endif

#endif
