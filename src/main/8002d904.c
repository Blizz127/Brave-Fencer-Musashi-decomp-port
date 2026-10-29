/* Owner re-derivation from retail assembly (clean input set). */
/* Plays music track <id> (0x100-based): resumes it with a fade-in if it is the parked track, else starts its sequence from the loaded sound bank, then sets volume, starts the player and marks the music flags. */
#include "psx_types.h"

typedef struct {
    s16 bank;  /* sound bank record (D_800A4640, 24 bytes each) */
    u16 vol;
    s16 prog;  /* index into D_80065438 */
    s16 seq;   /* slot in the bank's sequence offset table, 0 = default sequence */
    s16 f8;
    u16 flags; /* bit 0: loops */
    u8 padC[4];
} Track;

/* Same storage as D_800A4E7C..D_800A4EA3, viewed as one record. */
typedef struct {
    u32 bank; /* guest address of the loaded sound bank */
    u8 pad4[4];
    s16 vol;
    s16 handle; /* sequence player index, -1 = none */
    u8 pad88[2];
    s16 fade;
    u16 track;
    u16 flags;
    u8 pad90[0x1E - 0x14];
    u16 parkHandle;
    u8 pad9C[0x24 - 0x20];
    u16 parkTrack;
    u16 parkFlags;
} Snd;

extern void func_8002EE64();
extern void func_8003819C(s16, s32, s32, s16);
extern s32 func_80038210(u8 *, s16, s32);
extern void func_80038308(s16);
extern void func_8003834C(s16, s16);
extern void func_80038638(s16, s32);
extern s32 D_800652F0[];
extern u16 D_80065438[];
extern Track D_80068B5C[];
extern u32 D_800760E0;
extern s16 D_800A4642[][12];
extern Snd D_800A4E7C;
extern u8 D_800A4F1C;
extern u8 D_800A4F1E;

void func_8002D904(u16 id)
{
    s32 idx;
    s16 h;
    u32 data;
    s32 off;
    s32 prog;
    s32 player;
    u16 *park;
    s16 *hp;
    u16 *fp;
    s16 *hp2;

    idx = id - 0x100;
    func_8002EE64();
    h = D_800A4642[D_80068B5C[idx].bank][0];
    if (h < 0) return;
    if (!(D_800A4E7C.flags & 0x20)) return;
    if (D_800A4F1E != 0) return;
    if (id == 0x18E) {
        data = D_800760E0;
    } else if (D_80068B5C[idx].seq != 0) {
        off = ((s32 *)D_800A4E7C.bank)[D_80068B5C[idx].seq];
        if (off == 0) return;
        data = D_800A4E7C.bank + off + 4;
    } else {
        data = D_800A4E7C.bank + 0x10;
    }
    park = &D_800A4E7C.parkFlags;
    prog = D_800652F0[D_80065438[D_80068B5C[idx].prog]];
    if ((*park & 4) && D_800A4E7C.parkTrack == idx) {
        D_800A4E7C.parkTrack = 0;
        D_800A4E7C.flags = *park;
        *park = 0;
        D_800A4E7C.flags |= 4;
        D_800A4E7C.track = idx;
        D_800A4E7C.fade = 0;
        D_800A4E7C.handle = D_800A4E7C.parkHandle;
        D_800A4E7C.vol = D_80068B5C[D_800A4E7C.track].vol;
        func_8003819C(D_800A4E7C.handle, 0, 0x4000, 0x111);
        func_8003834C(D_800A4E7C.handle, D_800A4E7C.vol);
        func_80038308(D_800A4E7C.handle);
        D_800A4E7C.flags |= 9;
        return;
    }
    player = func_80038210((u8 *)data, h, prog);
    hp = &D_800A4E7C.handle;
    *hp = player;
    if (*hp == -1) return;
    if (id == 0x134) {
        if (!(D_800A4F1C & 2)) func_80038638(*hp, 4);
        if (!(D_800A4F1C & 4)) {
            func_80038638(*hp, 5);
            func_80038638(*hp, 6);
            func_80038638(*hp, 7);
        }
        if (!(D_800A4F1C & 8)) func_80038638(*hp, 8);
    }
    fp = &D_800A4E7C.flags;
    *fp = (*fp & 0x20) | 4;
    D_800A4E7C.vol = D_80068B5C[idx].vol;
    D_800A4E7C.track = idx;
    if (D_80068B5C[idx].flags & 1) {
        *fp |= 0x100;
    } else {
        *fp &= ~0x100;
    }
    hp2 = &D_800A4E7C.handle;
    func_8003834C(*hp2, D_800A4E7C.vol);
    D_800A4E7C.fade = 0;
    func_80038308(*hp2);
    D_800A4E7C.flags |= 9;
}
