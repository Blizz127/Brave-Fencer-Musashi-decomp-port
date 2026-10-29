/* func_8013A8FC - clean-room owner re-derivation (nmG). */
/* Tries to bind a free slot (first of the 2 x 0x10 entries of D_8011DA80 whose leading half is 0) to o via
 * func_8013A9B4. Returns 1 if o is already idle (+0x18 == 0) or the bind happened, else 0. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x18];
    u16 count; /* 0x18 */
    u8 pad1A[0x26];
    u32 slot;  /* 0x40 */
} Obj8013A8FC;

typedef struct {
    s16 id;
    u8 pad02[0xE];
} Slot8013A8FC; /* 0x10 bytes */

extern u8 D_8011DA80[];
extern void func_8013A9B4(Obj8013A8FC *o, Slot8013A8FC *slot);

s32 func_8013A8FC(Obj8013A8FC *o)
{
    s32 off;
    s32 n;
    Slot8013A8FC *e;

    if (o->count == 0) {
        return 1;
    }
    if (o->slot == 0) {
        off = 0;
        do {
            if (*(s16 *)(D_8011DA80 + off) == 0) {
                e = (Slot8013A8FC *)(D_8011DA80 + off);
                goto found;
            }
            off += 0x10;
        } while (off < 0x20);
        e = 0;
found:
        if (e == 0) {
            return 0;
        }
        n = o->count;
        if (n < 7) {
            if (n == 0) {
                return 0;
            }
            func_8013A9B4(o, e);
            return 1;
        }
    }
    return 0;
}
