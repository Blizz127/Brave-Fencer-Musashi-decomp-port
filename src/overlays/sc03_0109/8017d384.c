#include "../mine_entrance_camera.h"

/* This entrance selects slot zero in the mounted room. */
extern SceneCameraHandler D_80181E50[];

void func_8017D384(FacingActor *camera)
{
    D_80181E50[camera->mode](camera);
}
