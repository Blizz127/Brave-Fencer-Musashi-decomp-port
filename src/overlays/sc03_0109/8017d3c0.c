#include "../mine_camera_target.h"

extern void func_8017D48C(FacingActor *camera, SceneVector *anchor);

void func_8017D3C0(FacingActor *camera)
{
    SceneVector anchor = CAMERA_ANCHOR();
    if (anchor.position.y < -1664) {
        if (anchor.position.x < -736) anchor.position.x = -736;
        if (anchor.position.x > 704) anchor.position.x = 704;
    } else {
        if (anchor.position.x < -576) anchor.position.x = -576;
        if (anchor.position.x > 576) anchor.position.x = 576;
    }
    func_8017D48C(camera, &anchor);
}
