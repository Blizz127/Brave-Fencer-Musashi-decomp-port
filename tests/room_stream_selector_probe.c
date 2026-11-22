#include <assert.h>
#include <string.h>
#include "../src/overlays/main_0010/800cfe60.c"

RoomState D_800AF630;
s16 D_800B9A08;
u32 D_800D34AC[4096];
s16 D_800D34AE[8192];
u32 D_800C7C60;
u32 D_800C7C64[2];
u8 D_800A2E20[1];
DirectoryRow D_800AE830[4096];
s32 D_80072C74;

static const int ordinary_row = 22;
static const int ordinary_track = 9;
static int group, active_track, progress, flag_value, submit_result;
static int room_after_group, room_after_submit;
static int expected_flag, expected_row, expected_track, expected_size;
static int expected_commit, expected_track_argument;
static const u32 prior_bank = 733;
static char calls[16];
static unsigned call_count;

static void record(char code)
{
    assert(call_count + 1 < sizeof(calls));
    calls[call_count++] = code;
    calls[call_count] = 0;
}

static void select_room(int room)
{
    D_800AF630.room = D_800B9A08 = (s16)room;
}

static void descriptor(int room, unsigned next_group)
{
    unsigned slot = ROOM_INDEX(room);
    D_800D34AC[slot] = (unsigned)ordinary_track << 16 | next_group << 12 | (unsigned)ordinary_row;
    D_800D34AE[slot * 2] = (s16)ordinary_track;
}

s32 func_8002F648(void)
{
    record('g');
    if (room_after_group) {
        select_room(room_after_group);
    }
    return group;
}

void func_8002F620(void) { record('r'); }
s32 func_80029504(void) { record('p'); return progress; }
s32 func_80029178(u32 flag) { record('f'); assert(flag == (u32)expected_flag); return flag_value; }
s32 func_8002F658(void) { record('t'); return active_track; }

s32 func_80019A24(DirectoryRow *row, s32 destination, s32 size, u32 *track)
{
    record('s');
    assert(row == &D_800AE830[expected_row] && row->first && row->second == 77);
    assert(destination == 901 && size == expected_size);
    assert(track == (expected_track_argument ? &D_800C7C60 : 0));
    if (track) {
        assert(*track == (u32)expected_track && D_800C7C64[0] == (u32)(PointerBits)D_800A2E20);
    }
    if (room_after_submit) {
        select_room(room_after_submit);
    }
    return submit_result;
}

s32 func_800D00E4(s32 value) { record('c'); assert(value == expected_commit); return 99; }

static void reset(int room)
{
    memset(&D_800AF630, 0, sizeof(D_800AF630));
    memset(D_800D34AC, 0, sizeof(D_800D34AC));
    memset(D_800D34AE, 0, sizeof(D_800D34AE));
    memset(D_800AE830, 0, sizeof(D_800AE830));
    call_count = 0;
    calls[0] = 0;
    select_room(room);
    descriptor(room, 3);
    D_800C7C60 = 203;
    D_800C7C64[0] = prior_bank;
    D_800C7C64[1] = 1771;
    D_80072C74 = 901;
    group = 3;
    active_track = ordinary_track;
    progress = flag_value = room_after_group = room_after_submit = 0;
    expected_row = ordinary_row;
    expected_track = ordinary_track;
    expected_size = -1;
    expected_flag = expected_commit = -1;
    expected_track_argument = 0;
    submit_result = 7;
}

static void request(const char *sequence, int return_value, int present)
{
    D_800AE830[expected_row].first = (u32)present;
    D_800AE830[expected_row].second = 77;
    assert(func_800CFE60() == return_value);
    assert(strcmp(calls, sequence) == 0);
    assert(D_800C7C64[1] == 1771);
    assert(D_800C7C60 == (expected_track_argument ? (u32)expected_track : 203u));
    assert(D_800C7C64[0] == (expected_track_argument ? (u32)(PointerBits)D_800A2E20 : prior_bank));
}

int main(void)
{
    static const struct {
        int room, branch, phase, flag, value, row, track;
        const char *calls;
    } overrides[] = {
        {0x300B, 1, 0, -1, 0, 138, 8, "gpts"},
        {0x300B, 0, 0, -1, 0, 22, 9, "gpts"},
        {0x3001, 0, 4, -1, 0, 55, 9, "gpts"},
        {0x3001, 0, 5, -1, 0, 22, 9, "gpts"},
        {0x304E, 0, 0, 23, 1, 297, 47, "gpfts"},
        {0x304E, 0, 0, 23, 0, 22, 9, "gpfts"},
        {0x304E, 0, 0, 23, 256, 22, 9, "gpfts"},
        {0x304E, 0, 0, 23, 257, 297, 47, "gpfts"},
        {0x1077, 0, 679, -1, 0, 22, 9, "gpts"},
        {0x1077, 0, 680, -1, 0, 337, 52, "gpts"},
        {0x3021, 0, 0, 185, 255, 193, 65, "gpfts"},
        {0x3021, 0, 0, 185, 0, 22, 9, "gpfts"}
    };
    unsigned i;
    int changed, playing, present, result_index;
    const int results[] = {0, 3, -2, -1};

    for (i = 0; i < sizeof(overrides) / sizeof(overrides[0]); ++i) {
        reset(overrides[i].room);
        D_800AF630.branch = (s16)overrides[i].branch;
        progress = overrides[i].phase;
        expected_flag = overrides[i].flag;
        flag_value = overrides[i].value;
        expected_row = overrides[i].row;
        expected_track = overrides[i].track;
        expected_track_argument = expected_track != active_track;
        request(overrides[i].calls, submit_result, 1);
    }

    for (changed = 0; changed < 3; ++changed) {
        for (playing = 0; playing < 2; ++playing) {
            for (present = 0; present < 2; ++present) {
                for (result_index = 0; result_index < 4; ++result_index) {
                    char sequence[12] = "g";
                    int returned;
                    reset(0x2016);
                    group = changed == 0 ? 3 : changed == 1 ? 0 : 2;
                    active_track = playing ? ordinary_track : 31;
                    expected_size = changed ? 0 : -1;
                    expected_track_argument = changed || !playing;
                    submit_result = results[result_index];
                    returned = present ? submit_result : 1;
                    if (group == 2) strcat(sequence, "r");
                    strcat(sequence, present ? "pts" : "pt");
                    if (changed && (returned & 1)) {
                        expected_commit = 3;
                        strcat(sequence, "c");
                    }
                    request(sequence, returned, present);
                }
            }
        }
    }

    reset(0x2016);
    room_after_group = 0x202A;
    descriptor(room_after_group, 6);
    group = 6;
    request("gpts", submit_result, 1);

    reset(0x2016);
    room_after_submit = 0x202B;
    descriptor(room_after_submit, 6);
    group = 2;
    expected_size = 0;
    expected_track_argument = 1;
    expected_commit = 6;
    request("grptsc", submit_result, 1);
    return 0;
}
