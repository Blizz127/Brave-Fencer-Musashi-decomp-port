/* Scale the active dialog band colors for this scene. */
#include "mine_dialog_band.h"

void func_801299C8(s16 enabled, s16 selector, u8 *band)
{
    s32 red, green, blue;

    if (enabled == 0)
        return;

    switch (selector) {
    case 1:
        red = D_801A0D8A;
        green = D_801A0CB2;
        blue = D_801A0C90;
        D_8019F950[0].red = (10 * red) >> 4;
        D_8019F950[0].green = (128 * green) >> 4;
        D_8019F950[0].blue = (255 * blue) >> 4;
        D_8019F950[1].red = (143 * red) >> 4;
        D_8019F950[1].green = (200 * green) >> 4;
        D_8019F950[1].blue = (255 * blue) >> 4;
        D_8019F950[2].red = (255 * red) >> 4;
        D_8019F950[2].green = (255 * green) >> 4;
        D_8019F950[2].blue = (255 * blue) >> 4;
        D_8019F950[3].red = (143 * red) >> 4;
        D_8019F950[3].green = (255 * green) >> 4;
        D_8019F950[3].blue = (180 * blue) >> 4;
        break;
    case 0:
    case 2:
        D_8019F950[selector].red = (band[68] * D_801A0D8A) >> 4;
        D_8019F950[selector].green = (band[69] * D_801A0CB2) >> 4;
        D_8019F950[selector].blue = (band[70] * D_801A0C90) >> 4;
        D_8019F950[selector + 1].red = (band[71] * D_801A0D8A) >> 4;
        D_8019F950[selector + 1].green = (band[72] * D_801A0CB2) >> 4;
        D_8019F950[selector + 1].blue = (band[73] * D_801A0C90) >> 4;
        break;
    case 3:
    case 4:
        band[32] = D_801A0D8A << 3;
        band[33] = D_801A0CB2 << 3;
        band[34] = D_801A0C90 << 3;
        break;
    }
}
