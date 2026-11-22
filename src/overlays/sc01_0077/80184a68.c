/* Start the next animation within the placement's approach distance, then check retirement. */
#include "psx_types.h"
extern s32 func_8012BCCC(u8 *actor);
extern s32 func_8012C044(u8 *actor);
extern void func_8012C098(u8 *actor);
extern void func_80184554(u8 *actor);

void func_80184A68(u8 *actor)
{
    if (*(s16 *)(actor + 0x70) & 0x8000) {
        if (func_8012BCCC(actor) <= 0x90000)
            func_80184554(actor);
    } else if (func_8012BCCC(actor) <= 0x24000) {
        func_80184554(actor);
    }
    if (func_8012C044(actor))
        func_8012C098(actor);
}
