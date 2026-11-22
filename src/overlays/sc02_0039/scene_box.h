#ifndef BFM_SC02_SCENE_BOX_H
#define BFM_SC02_SCENE_BOX_H

#include "model_surface_probe.h"

extern u32 D_8017F68C, D_8017F690, D_8017F694, D_8017F698;
extern SceneProbeVector D_80126720;

#ifdef __mips__
#define box_point(address) ((SceneProbeVector *)(address))
#else
#include <stdint.h>
/* Host operations require backing aligned for the declared C object. */
_Noreturn void bfm_model_box_invalid_point(u32 address);
_Noreturn void bfm_model_box_invalid_bounds(void);
_Noreturn void bfm_model_box_divide_by_zero(void);

static inline SceneProbeVector *box_point(u32 address)
{
    SceneProbeVector *point = bfm_model_probe_point(address);
    if (point == 0 || (uintptr_t)point % _Alignof(SceneProbeVector))
        bfm_model_box_invalid_point(address);
    return point;
}
#endif

s32 func_80135EB0(SceneSurfaceBox *box, s32 flags);
s32 func_80136334(SceneSurfaceBox *box, s32 flags, s16 face);
s32 func_801365B8(SceneSurfaceBox *box, s32 flags, s16 face);
s32 func_80136824(SceneSurfaceBox *box, s32 flags, s16 face);

#endif
