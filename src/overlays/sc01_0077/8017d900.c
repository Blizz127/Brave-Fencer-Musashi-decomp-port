/* Owner re-derivation from retail assembly (clean input set). */
/* Sets the camera-window words and halfwords D_80126950.. and calls func_80174E9C with the table D_801B1DAC. */
#include "psx_types.h"

/* The camera setup routine stores a PS1 data address. */
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern u8 D_801B1DAC[];
#define OPENING_CAMERA_RECORD ((u32)D_801B1DAC)
#else
#define OPENING_CAMERA_RECORD 0x801B1DACU
#endif

extern s32 D_80126950;
extern s32 D_80126954;
extern s32 D_8012695C;
extern s16 D_80126968;
extern s16 D_8012696A;
extern s16 D_8012696C;
extern s16 D_80126976;
extern s16 D_80126978;
extern s16 D_8012697A;
extern void func_80174E9C(u32 table);

void func_8017D900(void)
{
    D_80126950 = 0x1F4;
    D_80126954 = 0x1F4;
    D_8012695C = 0x320;
    D_80126968 = 0x60;
    D_8012696A = 0x800;
    D_80126976 = 0x18;
    D_80126978 = -0x80;
    D_8012696C = 0;
    D_8012697A = -0x7D;
    func_80174E9C(OPENING_CAMERA_RECORD);
}
