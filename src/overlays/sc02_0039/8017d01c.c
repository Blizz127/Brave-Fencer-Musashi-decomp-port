#include "camera.h"

extern s32 func_8012A018(u32 callback_address, s32 mode);
extern s32 func_8012A094(u8 *record);
extern FacingActor D_80126948;
extern s16 D_801A0C88;

/* The original compiler emits one relocation per camera field. The host
 * owner keeps those fields in the same record passed to the callbacks. */
#if defined(__mips__) && defined(__LITTLE_ENDIAN__) && defined(__GNUC__) && __GNUC__ == 2
extern s32 D_80126950, D_80126954, D_8012695C;
extern s16 D_80126968, D_8012696A, D_8012696C;
extern s16 D_80126976, D_80126978, D_8012697A, D_801269E8;
#define CAMERA_CALLBACK ((u32)func_8017D0D4)
#define DISTANCE D_80126950
#define TARGET_DISTANCE D_80126954
#define PHASE_VALUE D_8012695C
#define ROTATION_X D_80126968
#define ROTATION_Y D_8012696A
#define ROTATION_Z D_8012696C
#define OFFSET_X D_80126976
#define OFFSET_Y D_80126978
#define OFFSET_Z D_8012697A
#define FACING D_801269E8
#else
#define CAMERA_CALLBACK 0x8017D0D4u
#define DISTANCE D_80126948.distance
#define TARGET_DISTANCE D_80126948.target_distance
#define PHASE_VALUE D_80126948.phase_value
#define ROTATION_X D_80126948.target_rotation.position.x
#define ROTATION_Y D_80126948.target_rotation.position.y
#define ROTATION_Z D_80126948.target_rotation.position.z
#define OFFSET_X D_80126948.target_offset.x
#define OFFSET_Y D_80126948.target_offset.y
#define OFFSET_Z D_80126948.target_offset.z
#define FACING D_80126948.facing
#endif

s32 func_8017D01C(void)
{
    s32 facing;

    DISTANCE = TARGET_DISTANCE = 400;
    PHASE_VALUE = 1000;
    ROTATION_X = 227;
    ROTATION_Y = 0;
    ROTATION_Z = 0;
    OFFSET_X = 0;
    OFFSET_Y = -80;
    OFFSET_Z = 0;
    func_8012A018(CAMERA_CALLBACK, 0);
    func_8012A094((u8 *)&D_80126948);
    func_8017D0D4(&D_80126948);
    func_8012A094((u8 *)&D_80126948);
    facing = (u16)FACING;
    D_801A0C88 = facing;
    return facing;
}
