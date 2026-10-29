/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame sound channel step: delays, release/fade, pitch slide, pan/volume refresh into the SPU voice records, timeouts. */
#include "psx_types.h"

/* One sound channel record (0x54 bytes). */
typedef struct {
    s32 delay;
    s32 ticks;
    u8 pad8[2];
    u16 id;
    u8 padC[6];
    u16 timeout;
    u8 pad14[4];
    s32 val;
    s32 target;
    u8 pad20[0x12];
    u16 step;
    u8 volIdx;
    u8 pan;
    u8 pad36;
    u8 flags;
    u8 pad38[8];
    u32 cb;         /* guest function address */
    u32 cbArg;
    s16 vol;
    s16 fadeLvl;
    u8 fade;
    u8 pad4D;
    u8 state;
    u8 touch;
    u8 keyed;
    u8 cbId;
    u8 lastPan;
    u8 panOfs;
} SndChan;

/* SPU voice shadow record (0x48 bytes). */
typedef struct {
    u32 mask;
    u32 flags;
    u16 volL;
    u16 volR;
    u8 padC[0x34];
    s32 voice;
    u8 release;
    u8 keyOff;
    u8 pad46[2];
} SndVoice;

/* Sound work area at 0x800A46E8 (only the parts this function uses). */
typedef struct {
    u8 pad0[0x2A0];
    SndChan ch[8];      /* 0x2A0 = D_800A4988 */
    SndVoice voice[8];  /* 0x540 = D_800A4C28 */
    u8 pad780[0x92];
    s16 master;         /* 0x812 = D_800A4EFA */
    u8 pad814[2];
    u8 mode[0x1B];      /* 0x816 = D_800A4EFE */
    u8 stereo;          /* 0x831 = D_800A4F19 */
} SndWork;

typedef void (*SndCb)(s32, s32);

extern SndWork D_800A46E8;
/* const: read-only tables (lets the scheduler move stores past their loads, as retail). */
extern const u32 D_80073140[];
extern const u16 D_8006AA30[];
extern const u16 D_800731A0[];
extern u32 D_800C7D2C;
extern u8 D_800A4F19; /* = D_800A46E8.stereo, plain-scalar view */

void func_8002EFF8(s32 mode, s32 bits);
void func_800316F8(SndChan *ch);
void func_800314DC(SndChan *ch);
void func_80031988(SndChan *ch);
void func_8003D3B4(s32 voice, s32 mode);

void func_80030F80(void)
{
    s32 i;
    SndChan *ch;
    SndVoice *v;
    u8 changed;
    u8 st;
    u16 id;
    u8 m;
    s16 pan;
    s16 t;
    u32 vol;
    u8 *mode;
    s32 goal;
    s32 d;

    mode = D_800A46E8.mode;
    ch = D_800A46E8.ch;
    for (i = 0; i < 8; i++, ch++) {
        changed = 0;
        ch->state &= 0x7F;
        if (ch->state != 0) {
            ch->ticks++;
            if (ch->delay != 0) {
                if (--ch->delay != 0) {
                    continue;
                }
                func_8002EFF8(1, D_80073140[ch->id]);
                func_800316F8(ch);
                continue;
            }
            st = ch->state;
            if (st >= 2) {
                ch->state = st - 1;
            } else {
                id = ch->id;
                switch (mode[id]) {
                case 3:
                    ch->keyed = 0;
                    func_8002EFF8(0, D_80073140[ch->id]);
                    /* fall through */
                case 0:
                    ch->state = 0;
                    if (ch->cb) {
                        ((SndCb)ch->cb)(ch->cbId, ch->cbArg);
                    }
                    ch->cb = 0;
                    break;
                case 2:
                    if (ch->keyed) {
                        func_8003D3B4(id, 1);
                        ch->keyed = 0;
                    }
                    break;
                }
            }
            if (ch->fade) {
                ch->fadeLvl -= 0x220;
                changed = 1;
                if (ch->fadeLvl <= 0) {
                    ch->fadeLvl = 0;
                    ch->fade = 0;
                    ch->keyed = 1;
                    D_800A46E8.voice[i].keyOff = 1;
                    func_8002EFF8(0, D_80073140[ch->id]);
                }
            }
            if (ch->touch) {
                changed = 1;
                ch->touch = 0;
            }
            if (ch->flags & 2) {
                /* d holds the new value, then the still-moving flag (one variable, as retail). */
                if (ch->target >= 0) {
                    d = ch->val + ch->step;
                    ch->val = d;
                    goal = ch->target;
                    d = d < goal;
                } else {
                    d = ch->val - ch->step;
                    ch->val = d;
                    goal = ch->target;
                    d = goal < d;
                }
                changed = 1;
                if (!d) {
                    ch->val = goal;
                    ch->flags &= ~2;
                }
            }
            if ((D_800A4F19 && ch->panOfs) || changed) {
                pan = ch->pan;
                if (pan) {
                    pan += ch->val >> 8;
                    if (ch->panOfs) {
                        t = ch->panOfs + pan;
                        if (t < 0x42) {
                            pan = 1;
                        } else {
                            pan = t - 0x40;
                            if (pan >= 0x80) {
                                pan = 0x7F;
                            }
                        }
                    }
                }
                if (changed || pan != ch->lastPan) {
                    vol = D_8006AA30[ch->volIdx];
                    vol = (vol * D_800A46E8.master) >> 7;
                    vol = (vol * ch->vol) >> 7;
                    if (changed) {
                        vol = (vol * (ch->fadeLvl >> 7)) >> 8;
                    }
                    v = &D_800A46E8.voice[i];
                    v->mask = D_80073140[ch->id];
                    if (pan) {
                        if (D_800A46E8.stereo) {
                            v->volL = (vol * D_800731A0[0x7F - pan]) >> 14;
                            v->volR = (vol * D_800731A0[pan - 1]) >> 14;
                        } else {
                            v->volL = v->volR = (vol * D_800731A0[0x3F]) >> 14;
                        }
                    } else {
                        v->volL = v->volR = vol;
                    }
                    v->voice = ch->id;
                    if (v->release) {
                        v->flags |= 3;
                    } else {
                        v->release = 1;
                        v->flags = 3;
                    }
                    ch->lastPan = pan;
                }
            }
            func_800314DC(ch);
            if (ch->timeout) {
                if (--ch->timeout == 0) {
                    func_80031988(ch);
                    ch->keyed = 0;
                    func_8002EFF8(0, D_80073140[ch->id]);
                    if (ch->cb) {
                        ((SndCb)ch->cb)(ch->cbId, ch->cbArg);
                    }
                    ch->cb = 0;
                }
            }
        } else {
            id = ch->id;
            m = mode[id];
            if (m == 0 || m == 3) {
                D_800C7D2C &= ~D_80073140[id];
            }
        }
    }
}
