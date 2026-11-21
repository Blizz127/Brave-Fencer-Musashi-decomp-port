#include "psx_types.h"

extern u32 D_8012697C;
extern u16 D_80126980;
extern s32 func_8012A018(u32 callback_address, s32 mode);

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern void func_80174F28(void *);
#define OPENING_CAMERA_CALLBACK ((u32)func_80174F28)
#else
#define OPENING_CAMERA_CALLBACK 0x80174F28u
#endif

void func_80174E9C(u32 table_address)
{
    D_8012697C = table_address;
    D_80126980 = 0;
    func_8012A018(OPENING_CAMERA_CALLBACK, 0);
}
