/* Owner re-derivation from retail assembly (clean input set). */
/* Steps the two 16-byte fade slots at D_8011DA80, dispatching on each slot's state. */
#include "psx_types.h"

typedef struct {
    /* 0x00 */ s16 state;
    /* 0x02 */ u8 pad02[0xE];
} Slot8013A380; /* size 0x10 */

extern Slot8013A380 D_8011DA80[];

extern void func_8013A448(Slot8013A380 *slot);
extern void func_8013A4C4(Slot8013A380 *slot);
extern void func_8013A530(Slot8013A380 *slot);

void func_8013A380(void)
{
    Slot8013A380 *slot;
    s32 i;

    slot = D_8011DA80;
    for (i = 0; i < 2; i++, slot++) {
        if (slot != 0) {
            switch (slot->state) {
            case 2:
                func_8013A530(slot);
                break;
            case 1:
                func_8013A448(slot);
                break;
            case 3:
                func_8013A4C4(slot);
                break;
            }
        }
    }
}
