/* Owner re-derivation from retail assembly (clean input set). */
/* Throttled particle spawn: runs only once every (1<<bits) calls (tested against o's low counter
 * bits), via func_8012913C(0x22). Each of the first 3 result fields is base[i] + a random
 * remainder (func_8005C49C() % (2*interval[i])) - interval[i]; the 4th is a plain copy. Also
 * clears three words and stamps a fixed value through the new object's +0x20 pointer. Always
 * increments o's counter. */
#include "psx_types.h"

typedef struct {
    u16 f0;
} SrcObj;

typedef struct {
    s16 i0;
    s16 i1;
    s16 i2;
} Interval;

typedef struct {
    s16 b0;
    s16 b1;
    s16 b2;
    s16 b3;
} Base;

typedef struct {
    u8 pad0[0x6];
    s16 f6;  /* 0x6 */
    u8 pad8[0xA - 0x8];
    s16 fA;  /* 0xA */
    u8 padC[0xE - 0xC];
    s16 fE;  /* 0xE */
    u32 f10; /* 0x10 */
    u32 f14; /* 0x14 */
    u32 f18; /* 0x18 */
    u8 pad1C[0x20 - 0x1C];
    u8 *f20; /* 0x20 */
    u8 pad24[0x34 - 0x24];
    s16 f34; /* 0x34 */
} NewObj;

extern NewObj *func_8012913C(s32 kind);
extern s32 func_8005C49C(void);

void func_80143640(SrcObj *o, Base *base, Interval *interval, s32 bits)
{
    s16 mask = (1 << bits) - 1;

    if ((o->f0 & mask) == 0) {
        NewObj *n = func_8012913C(0x22);

        if (n != 0) {
            s32 r;
            s16 iv;

            r = func_8005C49C();
            iv = interval->i0;
            n->f6 = base->b0 + r % (iv * 2) - iv;

            r = func_8005C49C();
            iv = interval->i1;
            n->fA = base->b1 + r % (iv * 2) - iv;

            r = func_8005C49C();
            iv = interval->i2;
            n->fE = base->b2 + r % (iv * 2) - iv;

            n->f18 = 0;
            n->f14 = 0;
            n->f10 = 0;
            n->f34 = base->b3;
            *(u16 *)(n->f20 + 0x2C) = 0xC008;
        }
    }

    o->f0 = o->f0 + 1;
}
