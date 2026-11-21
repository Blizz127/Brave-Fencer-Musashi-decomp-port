#include "fade_setup.h"

#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
extern u16 D_80126B58[];
#else
extern u8 D_80126B58[];
#endif

s32 func_8016EE40(u32 *record, u32 flags)
{
    u16 id;
    s32 result;

    func_8016EF78(record, flags);
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    id = D_80126B58[0];
#else
    id = (u16)(D_80126B58[0] | ((u16)D_80126B58[1] << 8));
#endif
    result = (u32)(id - 25) < 2;
    if (!result) {
        func_801599A4((u8 *)D_80126B58);
        result = func_80159B3C((u8 *)D_80126B58);
    }
    return result;
}
