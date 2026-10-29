/* Owner re-derivation from retail assembly (clean input set). */
/* Waits for the word at +0x34 to become nonzero, counting a 0x8000 timeout down in the frame; when the timeout runs
 * out forces the word to 1, flips the buffer index at +0x28 and copies the first two half-words of that buffer's
 * 8-byte record (record table at +0x18) to +0x2C/+0x2E. Clears the word at +0x34 on exit. The timeout counter is
 * volatile: retail stores it and reloads it for each test. */
#include "psx_types.h"

typedef struct {
    u16 a, b, c, d;
} Rec;

typedef struct {
    u8 pad0[0x18];
    Rec rec[2];
    s32 w28;
    u16 h2C;
    u16 h2E;
    u8 pad30[4];
    s32 w34;
} Dev;

void func_800D298C(Dev *o)
{
    volatile s32 timeout = 0x8000;

    if (o->w34 == 0) {
        do {
            if (--timeout == 0) {
                o->w34 = 1;
                o->w28 = (o->w28 == 0);
                o->h2C = o->rec[o->w28].a;
                o->h2E = o->rec[o->w28].b;
            }
        } while (o->w34 == 0);
    }
    o->w34 = 0;
}
