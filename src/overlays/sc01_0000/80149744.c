/* Own retail: ordered flag/global/halfword checks with a Boolean return. */
#include "psx_types.h"
typedef struct { u8 _0[0x44]; u32 f44; u8 _48[0x64]; u16 fAC; } Obj80149744;
extern u8 D_80078EBF;
s32 func_80149744(Obj80149744 *o)
{
#if defined(__GNUC__) && __GNUC__ == 2 && defined(__mips__)
    register u32 result __asm__("$2") = 0;
#else
    u32 result = 0;
#endif
    if (!(o->f44 & 4) && D_80078EBF >= 0x80) {
        result = o->fAC & 0x80;
        result = result != 0;
    }
    return (s32)result;
}
