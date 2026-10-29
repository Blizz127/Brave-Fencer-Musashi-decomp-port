/* Owner re-derivation from retail assembly and locally generated m2c. */
#include "psx_types.h"

typedef struct { u8 b; } BfmByteArg;

extern void func_80036EB4(s32, s32, s32, s32, BfmByteArg);
extern void func_8002EC10(void);

void func_8002EE64(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    BfmByteArg arg4;
    arg4.b = 0x88;
    func_80036EB4(arg0, arg1, arg2, arg3, arg4);
    func_8002EC10();
}
