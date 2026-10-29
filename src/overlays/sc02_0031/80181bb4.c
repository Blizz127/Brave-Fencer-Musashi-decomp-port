/* Owner re-derivation from retail assembly (clean input set). */
/* When the scene-state check passes: initialises the ten 0x18-byte slots of D_801B7840 with their index, stops sound 0x83E and resets the object's state. */
#include "psx_types.h"

typedef struct {
    u8 b[0x18];
} Slot;

extern Slot D_801B7840[];
extern s32 func_80181EF0(void);
extern void func_80181DEC(Slot *s, s32 i);
extern void func_8002D4C8(s32 a, s32 b);
extern void func_8012AD50(void *o);

void func_80181BB4(void *o)
{
    s32 i;
    Slot *s;

    if (func_80181EF0()) {
        for (i = 0, s = D_801B7840; i < 10; i++, s++) {
            func_80181DEC(s, i);
        }
        func_8002D4C8(0x83E, 0);
        func_8012AD50(o);
    }
}
