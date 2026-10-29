/* Owner re-derivation from retail assembly (clean input set). */
/* Starts a sound sequence from a bank entry in a free/stolen D_800A46E8 slot (or restarts the one playing it), or with mode bit 0x1000 re-targets the playing one's volume; returns slot+1 or 0. */
#include "psx_types.h"

typedef struct {
    s16 vol;
    s16 step;
    s16 target;
    u8 ramp;
    u8 pad7;
} Track;

typedef struct {
    s16 kind;
    u16 priority;
    u16 id;
    u16 sub_id;
    u16 hold;
    u8 padA[2];
    u32 data;
    u32 cursor;
    s16 f14;
    u8 stop;
    u8 f17;
    s16 f18;
    s16 f1A;
    Track track[3];
    s16 bank;
    u8 f36;
    u8 f37;
    u8 aux_target;
    u8 aux;
    u8 voice_on[8];
    u8 pad42[6];
    u8 f48;
    u8 pad49[7];
    s16 f50;
    s16 f52;
} Slot;

typedef struct {
    u32 data;
    u16 priority;
    u16 hold;
    u8 flags;
    u8 pad9[3];
} Entry;

typedef struct {
    s16 id;
    u8 pad2[0x18 - 2];
    u32 entries;
} Bank;

typedef struct {
    u8 pad0;
    u8 bank;
    u8 f2;
    u8 index;
} Request;

extern s32 func_800348A8(u32);
extern s32 func_8003310C(s32);
extern void func_80034650(Slot *, u8);
extern Entry D_80068A54[];
extern u8 D_8006AED8[];
extern Slot D_800A46E8[];
extern u32 D_800A4EE8;
extern s16 D_800A4EF0;

s32 func_80034314(u32 sound, Request *req, u32 mode)
{
    Entry *e;
    Slot *s;
    Track *t;
    u8 *p;
    s32 n;
    s32 k;
    s32 i;
    s32 j;
    s32 lo;
    u32 hi;
    u32 flags;
    s32 bank;

    hi = sound >> 16;
    flags = mode;
    lo = sound;
    bank = req->bank;
    if (bank == 0) {
        e = &D_80068A54[req->index];
    } else {
        if (D_800A4EE8 == 0) {
            return 0;
        }
        if (D_800A4EF0 != bank) {
            return 0;
        }
        e = &((Entry *)((Bank *)D_800A4EE8)->entries)[req->index];
    }
    k = func_800348A8(sound);
    if (k != 0) {
        n = k;
        k = n - 1;
        s = &D_800A46E8[k];
        if (flags & 0x1000) {
            s->track[0].ramp = 1;
            s->track[0].step = 0x100;
            s->track[0].target = (s32)((flags & 0x7F) * 0x3FFF) >> 7;
            if (flags & 0x2000) {
                s->aux_target = D_8006AED8[(flags >> 8) & 0xF];
            } else {
                s->aux_target = 0;
                s->aux = 0;
            }
            return n;
        }
        if (s->hold != 0) {
            return 0;
        }
        func_80034650(s, 0);
    } else {
        k = func_8003310C(e->priority);
        if (k == 0) {
            return 0;
        }
        n = k;
        k = n - 1;
        s = &D_800A46E8[k];
    }
    s->f37 = e->flags;
    s->bank = bank;
    s->data = e->data;
    s->priority = e->priority;
    s->cursor = s->data;
    s->id = lo;
    s->sub_id = hi;
    s->f14 = 1;
    s->f17 = req->f2;
    s->stop = 0;
    s->f18 = 0x7FFF;
    s->hold = e->hold;
    s->f50 = 0;
    s->f52 = 0;
    if (e->flags & 0x10) {
        s->f48 = 1;
    } else {
        s->f48 = 0;
    }
    if (e->flags & 4) {
        s->f1A = 0x400;
    } else if (e->flags & 8) {
        s->f1A = 0x200;
    } else {
        s->f1A = 0x5F;
    }
    s->f36 = 0;
    t = s->track;
    for (i = 0; i < 3; i++, t++) {
        t->step = 0x100;
        t->ramp = 0;
        if (i != 0) {
            t->vol = 0x3FFF;
            t->target = 0x3FFF;
        } else if (flags & 0x1000) {
            t->vol = (flags & 0x7F) * 0x3FFF >> 7;
            t->target = (flags & 0x7F) * 0x3FFF >> 7;
            if (flags & 0x2000) {
                s->aux = s->aux_target = D_8006AED8[(flags >> 8) & 0xF];
            }
        } else {
            t->vol = 0x3FFF;
            t->target = 0x3FFF;
            s->aux_target = 0;
            s->aux = 0;
        }
    }
    p = s->voice_on;
    for (j = 0; j < 8; j++) {
        *p++ = 0;
    }
    s->kind = 5;
    return n;
}
