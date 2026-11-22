#include "../mine_camera_target.h"

extern u8 D_80126B58[];
extern s16 D_80186FA0[];
extern u16 func_80148800(u8 *actor);
extern void func_8017F1FC(FacingActor *camera, SceneVector *anchor);

void func_8017F15C(FacingActor *camera)
{
    SceneVector anchor;
    if (func_80148800(D_80126B58) & 3) {
        camera->phase = (camera->phase + 1) & 1;
        camera->phase_value = D_80186FA0[camera->phase];
    }
    anchor = CAMERA_ANCHOR();
    func_8017F1FC(camera, &anchor);
}
