/* Set the opening follow camera parameters and register its guest callback. */
#include "psx_types.h"

extern s32 D_80126954;
extern s32 D_8012695C;
extern s16 D_80126968;
extern s16 D_8012696A;
extern s16 D_8012696C;
extern s16 D_80126976;
extern s16 D_80126978;
extern s16 D_8012697A;
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern void func_8017DA08(u8 *camera);
#define OPENING_CAMERA_CALLBACK ((u32)func_8017DA08)
#else
#define OPENING_CAMERA_CALLBACK 0x8017DA08U
#endif
extern s32 func_8012A018(u32 callback_address, s32 mode);

void func_8017D98C(void)
{
    D_80126954 = 0x15E;
    D_8012695C = 0x258;
    D_80126968 = 0x1C2;
    D_8012696A = 0x600;
    D_8012696C = 0;
    D_80126976 = 0x140;
    D_80126978 = 0;
    D_8012697A = 0x140;
    func_8012A018(OPENING_CAMERA_CALLBACK, 0);
}
