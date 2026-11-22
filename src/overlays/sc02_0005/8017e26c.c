#include "tower_postfire_camera.h"

u32 func_8017E26C(void)
{
    PostfireCameraRawPosition first = D_801E2D90;
    PostfireCameraRawPosition second = D_801E2D98;
    TOWER_CAMERA_RESULT(result);

    D_80126FB4 = 500;
    D_80126FE4 = first.x;
    D_80126FE8 = first.y;
    D_80126FEC = first.z;
    D_80126FF0 = second.x;
    D_80126FF4 = second.y;
    D_80126FF8 = second.z;
    func_80129FF4();
    result = func_8012A094((u8 *)&D_80126FA8);
    func_8012A100(3);
    return result;
}
