/* Owner re-derivation from retail assembly (clean input set). */
/* Steps a (value, duration) byte-pair script once per call. D_80184198 holds the guest address of the
 * script, D_8018419C the next entry index and D_8018419D the frames left in the current entry. When the
 * duration runs out the next entry's value is copied to D_801269D4/D_801269C8; an entry with duration 0
 * ends the script (outputs and index cleared, script pointer reset). */
#include "psx_types.h"

/* Script entry: value to publish and how many frames to hold it. */
typedef struct {
    s8 value;
    u8 time;
} Step;

extern u32 D_80184198;
extern u8 D_8018419C;
extern u8 D_8018419D;
extern s32 D_801269D4;
extern s32 D_801269C8;

void func_8013CA14(void)
{
    u32 *p = &D_80184198;

    if (*p != 0) {
        if (D_8018419D == 0) {
            u8 i = D_8018419C;
            s32 v = ((Step *)D_80184198)[i].value;
            u8 n;

            D_801269D4 = v;
            D_801269C8 = v;
            n = ((Step *)D_80184198)[i].time;
            D_8018419C = i + 1;
            D_8018419D = n;
            if (n == 0) {
                D_801269D4 = 0;
                D_801269C8 = 0;
                D_8018419C = 0;
                *p = 0;
            }
        } else {
            D_8018419D = D_8018419D - 1;
        }
    }
}
