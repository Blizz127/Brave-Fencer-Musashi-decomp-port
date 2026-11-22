#include "../mine_entrance_camera.h"

/* This entrance selects slot zero in the mounted room. */
extern SceneCameraHandler D_80186F9C[];

void func_8017F120(FacingActor *camera)
{
    D_80186F9C[camera->mode](camera);
}
