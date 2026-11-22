/* Rotate the mine contact and normal when needed, then add the model translation. */
#include "model_surface_probe.h"

extern u32 D_8017F694;
extern SceneProbeVector D_80126720;

#if BFM_MINE_DIRECT_GUEST_ADDRESS
#define contact_point(address) ((SceneProbeVector *)(address))
#define model_axis(record, axis) (*(u32 *)((record)->matrix + 20 + 4 * (axis)))
#else
#define contact_point(address) bfm_model_probe_point(address)
static u32 model_axis(const SceneSurfaceRecord *record, unsigned axis)
{
    const u8 *v = record->matrix + 20 + 4 * axis;
    return (u32)v[0] | ((u32)v[1] << 8) | ((u32)v[2] << 16) | ((u32)v[3] << 24);
}
#endif

extern SceneProbeVector *func_80048FBC(void *matrix, SceneProbeVector *in, SceneProbeVector *out);
extern SceneTransformVector *func_8004864C(SceneProbeVector *in, SceneTransformVector *out);

void func_80136A94(s32 rotate, SceneSurfaceRecord *record, SceneProbeVector *output, void *matrix)
{
    if (rotate != 0) {
        SceneTransformVector transformed;
        SceneProbeVector *point = contact_point(D_8017F694);
        SceneProbeVector *normal;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        if (point == 0)
            return;
#endif
        func_80048FBC(matrix, point, point);
        func_80048FBC(matrix, &D_80126720, &D_80126720);
        normal = &D_801152A8;
        func_8004864C(normal, &transformed);
        normal->x = (s16)transformed.x;
        D_801152A8.y = (s16)transformed.y;
        D_801152A8.z = (s16)transformed.z;
    }
    {
        SceneProbeVector *point = contact_point(D_8017F694);
        SceneProbeVector *auxiliary;

#if !BFM_MINE_DIRECT_GUEST_ADDRESS
        if (point == 0)
            return;
#endif
        output->x = (s16)((u16)point->x + model_axis(record, 0));
        output->y = (s16)((u16)point->y + model_axis(record, 1));
        output->z = (s16)((u16)point->z + model_axis(record, 2));
        auxiliary = &D_80126720;
        auxiliary->x = (s16)((u16)auxiliary->x + model_axis(record, 0));
        D_80126720.y = (s16)((u16)D_80126720.y + model_axis(record, 1));
        D_80126720.z = (s16)((u16)D_80126720.z + model_axis(record, 2));
    }
}
