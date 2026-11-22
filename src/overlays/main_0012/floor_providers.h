#ifndef BFM_FLOOR_PROVIDERS_H
#define BFM_FLOOR_PROVIDERS_H

#include "scene_object_probes.h"

s32 func_80134510(SceneProbeVector *probe);
s32 func_80135D20(u32 object_address, u32 geometry_reference, SceneProbeVector *probe);
s32 func_8014FFDC(Obj8014FE60 *actor, SceneProbeVector *best, SceneProbeVector *normal);
s32 func_80150150(Obj8014FE60 *actor, SceneProbeVector *probe);
s32 func_801502EC(void *actor, SceneProbeVector *best, SceneProbeVector *normal);
s32 func_80150460(void *actor, SceneProbeVector *probe);

#endif
