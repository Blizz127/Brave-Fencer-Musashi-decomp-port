#include "game_track_rows.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

u8 D_8006AEF4;
s32 D_800760F0, D_800760F4;
TrackControl D_800A4638;
TrackDefinition D_80064D44[6];
s32 D_80065504[5];
u32 D_8006A970[5];
s16 D_800C5328[8][2];

static TrackPair pairs[32];
static TrackRow rows[35];
static TrackRowWindows windows;
static int ready, starts, played, lookups, attempts, next_id;
static void reset(void)
{
    windows = (TrackRowWindows){
        .pair_base = 0x80064000, .row_base = 0x800a5000,
        .pairs = pairs, .pair_count = 32, .rows = rows, .row_count = 35,
        .definition_count = 6, .slot_table_count = 5, .owner_count = 8
    };
    bfm_bind_track_rows(&windows);
    ready = 1;
    starts = played = lookups = attempts = next_id = 0;
    memset(&D_800A4638, 0, sizeof(D_800A4638));
    D_800A4638.slot = 2;
    D_800A4638.track = 3;
    D_800A4638.handle = -7;
    D_800A4638.phase = 4;
    D_800A4638.slots[2].empty = 1;
    D_800760F0 = 54;
    D_800760F4 = 42;
    D_8006AEF4 = 0xff;
    D_80065504[2] = 21;
    D_8006A970[2] = windows.row_base + 2 * sizeof(TrackRow);
    D_80064D44[3] = (TrackDefinition){
        .pair_address=windows.pair_base + sizeof(TrackPair), .count=27,
        .kind=5, .value=9, .target=6
    };
    memset(rows, 0xbb, sizeof(rows));
    memset(D_800C5328, 0xff, sizeof(D_800C5328));
    for (int i = 1; i < 32; ++i)
        pairs[i] = (TrackPair){i - 1, i + 99};
}

s32 func_80042374(s32 arg)
{
    assert(arg == 0);
    ++attempts;
    return 0x41230000 | ready;
}

void func_80037D74(void)
{
    assert(D_8006AEF4 == 0xfc);
    ++starts;
}

s16 func_8003F144(s16 bank, u8 first, u8 second, TrackInfo *info)
{
    assert(bank == -7 && first == next_id && second == first + 100);
    ++lookups;
    ++next_id;
    info->scale = 150 + first;
    info->first = 0xef00 + first;
    info->second = 0xda80 + first;
    info->key = -200 + first;
    return first == 2 ? -2 : 0;
}

s32 func_8003F380(s16 bank, s16 key)
{
    assert(bank == -7);
    assert(key == -200 + next_id - 1);
    ++played;
    return key == -196 ? -1 : 80000 + key;
}

static void test_chunks(void)
{
    reset();
    ready = 0;
    assert(!func_80030730() && !func_80030A14());
    assert(!starts && !lookups && D_8006AEF4 == 0xff && D_800760F0 == 54);
    ready = 1;
    assert(!func_80030730());
    assert(starts == 1 && lookups == 10 && played == 9);
    assert(D_800760F0 == 10 && D_800760F4 == 11 && D_800A4638.phase == 5);
    assert(D_800A4638.slots[2].empty == 1 && D_800C5328[3][0] == -1);
    assert(!func_80030A14());
    assert(lookups == 20 && D_800760F0 == 20 && D_800760F4 == 1);
    assert(D_800A4638.phase == 5 && D_800A4638.slots[2].empty == 1);
    assert(func_80030A14() == 1);
    assert(lookups == 21 && played == 20 && starts == 1 && attempts == 5);
    assert(D_800760F4 == 0 && D_800A4638.phase == 0 && D_800A4638.slots[2].empty == 0);
    assert(D_800C5328[3][0] == 2 && D_800C5328[6][1] == 2);
    assert(D_800A4638.slots[2].other_type == 5 && D_800A4638.slots[2].other_track == 6 &&
           D_800A4638.slots[2].reserved_a == 9);
    TrackRow sentinel;
    memset(&sentinel, 0xbb, sizeof(sentinel));
    for (int j = 0; j < 35; j++) {
        int i = j - 2;
        TrackRow wanted = sentinel;
        if (i >= 0 && i < 21 && i != 2 && i != 4) {
            wanted.active = 1;
            wanted.scale = (150 + i) << 8;
            wanted.handle = 79800 + i;
            wanted.first = 0xef00 + i;
            wanted.second = 0xda80 + i;
        }
        assert(memcmp(&rows[j], &wanted, sizeof(wanted)) == 0);
    }
}

static void test_limits(void)
{
    reset();
    D_80064D44[3].count = 0;
    D_80064D44[3].pair_address = 0;
    D_8006A970[2] = 0;
    assert(func_80030730() == 1 && !lookups && !D_800A4638.slots[2].empty);
    assert(D_800C5328[3][0] == -1 && !D_800A4638.phase);
    reset();
    D_80065504[2] = -1;
    D_80064D44[3].kind = 0;
    assert(func_80030730() == 1 && !lookups && D_800C5328[3][0] == 2);
    assert(D_800C5328[6][1] == -1);
    reset();
    D_80065504[2] = 10;
    assert(func_80030730() == 1 && lookups == 10 && D_800760F0 == 0);
}

static void test_invalid(void)
{
    reset();
    windows.pair_count = 10;
    bfm_bind_track_rows(&windows);
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    windows.row_count = 11;
    bfm_bind_track_rows(&windows);
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    D_800A4638.slot = 5;
    assert(func_80030A14() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    D_800A4638.track = -1;
    assert(func_80030A14() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    D_80065504[2] = 1;
    D_80064D44[3].target = 8;
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    D_800760F0 = -1;
    assert(func_80030A14() == 0 && bfm_track_rows_failed() && !lookups);
    D_800760F0 = INT_MAX;
    assert(func_80030A14() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    D_8006A970[2] += 2;
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    windows.row_base = 0xfffffff4;
    windows.row_count = 2;
    D_8006A970[2] = windows.row_base;
    bfm_bind_track_rows(&windows);
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
    reset();
    D_8006A970[2] = windows.pair_base;
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
    bfm_bind_track_rows(NULL);
    assert(func_80030730() == 0 && bfm_track_rows_failed() && !lookups);
}

int main(void)
{
    assert(sizeof(void *) == 8);
    test_chunks();
    test_limits();
    test_invalid();
    puts("PASS 64-bit host track chunks, opaque-call outcomes, invalid data windows and indices");
    return 0;
}
