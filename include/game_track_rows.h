#ifndef GAME_TRACK_ROWS_H
#define GAME_TRACK_ROWS_H

#include "game_track_state.h"

typedef struct { u8 first, second; } TrackPair;
typedef struct {
    u32 pair_address;
    u8 count, pad5, kind, value, target, pad9[3];
} TrackDefinition;
typedef struct {
    s32 handle;
    u16 scale, first, second;
    u8 active, pad11;
} TrackRow;
typedef struct {
    u8 pad0[4], scale, pad5[11];
    u16 first, second, pad20;
    s16 key;
    u8 pad24[8];
} TrackInfo;
extern u8 D_8006AEF4;
extern s32 D_800760F0, D_800760F4;
extern TrackDefinition D_80064D44[];
extern s32 D_80065504[];
extern u32 D_8006A970[];
extern s16 D_800C5328[][2];
extern s32 func_80042374(s32 arg);
extern void func_80037D74(void);
extern s16 func_8003F144(s16 bank, u8 first, u8 second, TrackInfo *info);
extern s32 func_8003F380(s16 bank, s16 key);

s32 func_80030730(void);
s32 func_80030A14(void);

/* The pinned PS1 compiler runs a freestanding preprocessor. Host compilers
 * select the checked data path by default, including 64-bit MIPS hosts. */
#if !defined(__mips__) || defined(__SIZEOF_POINTER__)
#ifndef BFM_NATIVE_TRACK_ROWS
#define BFM_NATIVE_TRACK_ROWS 1
#endif
#endif

#ifdef BFM_NATIVE_TRACK_ROWS
#include <stddef.h>
/* Supply the actual typed game tables. The two address windows describe only
 * the current sound pair data and its destination rows. Addresses in the
 * sound definitions stay 32-bit on every host. */
typedef struct {
    u32 pair_base, row_base;
    const TrackPair *pairs;
    size_t pair_count;
    TrackRow *rows;
    size_t row_count;
    size_t definition_count, slot_table_count, owner_count;
} TrackRowWindows;

/* Per-thread binding. A null pointer clears it. The caller owns the typed
 * arrays for the duration of all game calls made with this binding. */
void bfm_bind_track_rows(const TrackRowWindows *windows);
int bfm_track_slot_valid(s16 slot);
int bfm_track_definition_valid(s16 track);
int bfm_track_owners_valid(s16 track, u8 kind, u8 target);
int bfm_track_chunk(u32 pair_address, u32 row_address, s32 first, s32 remaining,
                    const TrackPair **pairs, TrackRow **rows);

/* Retail treats any nonzero row return as success. Report a rejected input
 * through a separate sticky flag while returning pending to the game. Reset
 * the flag by binding a corrected window before retrying. */
int bfm_track_rows_failed(void);
s32 bfm_reject_track_rows(void);
#define D_800A46D0 (D_800A4638.track)
#else
/* The continuation uses a direct halfword relocation to TrackControl.track.
 * The PS1 linker resolves this to the field inside D_800A4638. */
extern s16 D_800A46D0;
#endif

#endif
