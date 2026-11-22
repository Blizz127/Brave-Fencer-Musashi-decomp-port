#include "../mine_entrance_camera.h"

void func_8017F07C(void)
{
    CAMERA_FIELD(D_80126950, distance) = CAMERA_FIELD(D_80126954, target_distance) = 400;
    CAMERA_FIELD(D_8012695C, phase_value) = 1000;
    CAMERA_FIELD(D_80126968, target_rotation.position.x) = 227;
    CAMERA_FIELD(D_8012696A, target_rotation.position.y) = 2048;
    CAMERA_FIELD(D_8012696C, target_rotation.position.z) = 0;
    CAMERA_FIELD(D_80126976, target_offset.x) = 0;
    CAMERA_FIELD(D_80126978, target_offset.y) = -32;
    CAMERA_FIELD(D_8012697A, target_offset.z) = 0;

    func_8012A018(CAMERA_TOKEN(func_8017F120, 0x8017F120u), 0);
    func_8012A094((u8 *)&D_80126948);
    func_8017F120(&D_80126948);
}
