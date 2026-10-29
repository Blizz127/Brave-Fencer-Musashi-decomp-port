/* Owner re-derivation from retail assembly (clean input set). */
/* Looks up a table entry from the high half of the argument, optionally clears a matching slot, then resets pending state. */
#include "psx_types.h"

typedef struct {
    u8 flags;
    u8 pad[3];
} Info;

typedef struct {
    u8 pad0[4];
    u16 id;
    u8 pad6[84 - 6];
} Slot;

typedef struct {
    Slot slots[24];
    u8 pad7E0[0x4F];
    u8 busy;    /* 0x82F = D_800A4F17 */
    u8 pending; /* 0x830 = D_800A4F18 */
} SlotTable;

extern SlotTable D_800A46E8;
extern u8 D_800A4F18;
extern s16 D_800A4EFC;
extern Info D_8006451C[];
extern s32 func_8002F4E4(Info *info);
extern void func_8003324C(s32 slot);
extern void func_80030F80(void);
extern void func_8002D320(void);
extern void func_80031BE0(void);

void func_8002D6D8(u32 arg)
{
    u8 *flag = &D_800A46E8.busy;
    u16 key;
    u16 hi;
    Info *info;
    Slot *base;

    *flag = 1;
    key = arg >> 16;
    hi = (u16)key;
    arg &= 0xFFFF;
    if (hi >= 0x100) {
        info = &D_8006451C[hi];
        if ((info->flags & 0x7F) == 6) {
            key = func_8002F4E4(info);
            if ((u16)key == 0) {
                return;
            }
        }
        if (arg != 0) {
            arg--;
            base = D_800A46E8.slots;
            if (base[arg].id == (u16)key) {
                func_8003324C(arg & 0xFFFF);
            }
            *flag = D_800A4F18;
            if (D_800A4F18 != 0) {
                if (D_800A4EFC != 0) {
                    D_800A4EFC--;
                }
                func_80030F80();
                func_8002D320();
                func_80031BE0();
                D_800A4F18 = 0;
                *flag = 0;
            }
        }
    }
}
