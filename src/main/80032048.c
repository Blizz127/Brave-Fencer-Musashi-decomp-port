/* Owner re-derivation from retail assembly (clean room, clean input set).
 * Starts a sequence request: picks its 20-byte descriptor (built-in table D_80068304 when the
 * request's bank byte is 0, otherwise the loaded bank at D_800A4EE8 if its id D_800A4EF0
 * matches), then with slot updates paused (D_800A4F17) restarts the D_800A46E8 slot already
 * playing `sound` (func_800331D4) or takes a free/stolen one (func_8003310C), fills its header
 * and hands it to func_80032A74. Mode 0xFFFF means "id in the high half, no flags". Mode bit
 * 0x4000 re-targets a playing slot through func_800335B8. Returns slot+1, or 0. */
#include "psx_types.h"

typedef struct {
    u8 flags;
    u8 bank;
    u8 group;
    u8 index;
} Request;

typedef struct {
    u16 priority;
    u8 pad2[0xC - 2];
    u16 hold;
    u8 padE[0x14 - 0xE];
} Desc;

typedef struct {
    u8 pad0[0x14];
    u32 groups;
} Bank;

typedef struct {
    s16 kind;
    u16 priority;
    u16 id;
    u16 sub_id;
    u16 hold;
    u8 state;
    u8 padB[0x54 - 0xB];
} Slot;

extern s32 func_800331D4(u32);
extern s32 func_8003310C(s32);
extern void func_800335B8(s32, u16);
extern void func_8003324C(u16);
extern void func_80032A74(Slot *, s32, Desc *, u16);
extern u32 D_80068304[];
extern Slot D_800A46E8[];
extern u32 D_800A4EE8;
extern s16 D_800A4EF0;
extern u8 D_800A4F17;

s32 func_80032048(u32 sound, Request *req, u16 mode)
{
    Desc *d;
    Slot *s;
    u8 *pause;
    u8 was_paused;
    u32 lo;
    u32 hi;
    u32 prio;
    s32 n;
    s32 k;
    s32 bank;
    s32 group;
    s32 index;

    group = req->group;
    bank = req->bank;
    index = req->index;
    hi = sound >> 16;
    if (bank == 0) {
        d = &((Desc *)D_80068304[group])[index];
    } else {
        if (D_800A4EF0 != bank) {
            return 0;
        }
        if (D_800A4EE8 == 0) {
            return 0;
        }
        d = (Desc *)((u32 *)((Bank *)D_800A4EE8)->groups)[group];
        d += index;
    }
    pause = &D_800A4F17;
    was_paused = *pause;
    *pause = 1;
    prio = d->priority;
    n = 0;
    if (mode == 0xFFFF) {
        mode = 0;
        lo = hi & 0xFFFF;
        hi = 0;
    } else {
        lo = sound;
    }
    k = func_800331D4(lo);
    if (k == 0) {
        if (mode & 0x4000) {
            if (!(mode & 0x3000)) {
                goto done;
            }
        } else if ((mode & 0x1000) && (mode & 0x7F) < 0x30) {
            prio >>= 1;
        }
        k = func_8003310C(prio);
        if (k == 0) {
            goto done;
        }
        n = k;
        k = n - 1;
        s = &D_800A46E8[k];
    } else {
        if (mode & 0x4000) {
            func_800335B8(k - 1, mode);
            goto done;
        }
        k = k - 1;
        s = &D_800A46E8[k];
        if (s->hold != 0) {
            goto done;
        }
        n = k + 1;
        func_8003324C(k);
    }
    s->kind = (req->flags & 0xC0) | 1;
    s->id = lo;
    s->sub_id = hi;
    s->priority = prio;
    s->hold = d->hold;
    s->state = 5;
    func_80032A74(s, k, d, mode);
done:
    D_800A4F17 = was_paused;
    return n;
}
