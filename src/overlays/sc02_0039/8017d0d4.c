#include "camera.h"

/* The scene initializes mode zero. Host callers supply the corresponding
 * native handler table and ensure that the selected slot is valid. */
extern SceneCameraHandler D_80182B78[];

void func_8017D0D4(FacingActor *actor)
{
    D_80182B78[actor->mode](actor);
}
