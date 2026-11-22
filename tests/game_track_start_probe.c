/* Synthetic game state, with observable sound services at the call boundary. */
#include <assert.h>
#include <string.h>

#include "../src/main/8002ff0c.c"

enum { SLOT_COUNT = 5, TRACK_COUNT = 17 };
enum { INPUT_HEADER = -12345, UPDATED_HEADER = -23456, PREVIOUS_HANDLE = 47 };

u8 D_80064D49[TRACK_COUNT][12], D_80064D4A[TRACK_COUNT][12], D_80064D4E[TRACK_COUNT][12];
TrackControl D_800A4638;
s16 D_800A4EF6, D_800C5328[TRACK_COUNT][2], D_800C532A[TRACK_COUNT][2];

static char calls[24];
static size_t call_count;
static int requested_slot, requested_track, previous_track, alternate_track;
static s32 expected_address, expected_header;
static s16 opened_handle;
static s32 poll_result, first_result, second_result;
static int mutate_release, mutate_phases, redirect_close;
static s32 close_argument;

static void record(char service)
{
    assert(call_count + 1 < sizeof(calls));
    calls[call_count++] = service;
    calls[call_count] = 0;
}

void func_80031A98(void)
{
    record('r');
    assert(D_800C5328[previous_track][0] == -1);
    if (mutate_release) {
        assert(D_800C532A[alternate_track][0] == -1);
        assert(D_800A4638.slots[requested_slot].other_type == 0);
        assert(D_800A4638.slots[requested_slot].other_track == 0);
        D_80064D49[requested_track][0] = (u8)((requested_slot + 1) % SLOT_COUNT);
        D_80064D4E[requested_track][0] = 93;
        D_800A4638.slots[requested_slot].address = expected_address;
    }
}

s16 func_80041A80(s32 header, s32 bank, s32 address)
{
    record('o');
    assert(header == expected_header && bank == -1 && address == expected_address);
    assert(D_800A4638.slot == requested_slot);
    assert(D_800A4638.track == requested_track && D_800A4638.header == header);
    assert(D_800A4638.slots[requested_slot].track == requested_track);
    assert(D_800A4638.slots[requested_slot].header == header);
    if (mutate_release)
        assert(D_800A4EF6 == 93);
    return opened_handle;
}

s32 func_800419B0(s32 handle)
{
    record('p');
    assert(handle == opened_handle && D_800A4638.handle == opened_handle);
    assert(D_800A4638.slots[requested_slot].handle == opened_handle);
    return poll_result;
}

s32 func_80030730(void)
{
    record('a');
    assert(D_800A4638.phase == 1);
    if (mutate_phases)
        D_800A4638.phase = 2;
    return first_result;
}

s32 func_80030A14(void)
{
    record('b');
    assert(D_800A4638.phase == 2);
    if (mutate_phases)
        D_800A4638.handle = -299;
    return second_result;
}

void func_800415A8(s32 handle)
{
    record('c');
    close_argument = handle;
    if (redirect_close) {
        D_800A4638.slot = (s16)((requested_slot + 1) % SLOT_COUNT);
        D_800A4638.handle = 309;
    }
}

static void reset(int slot)
{
    int i;
    memset(&D_800A4638, 0, sizeof(D_800A4638));
    memset(D_80064D49, 0, sizeof(D_80064D49));
    memset(D_80064D4A, 0, sizeof(D_80064D4A));
    memset(D_80064D4E, 0, sizeof(D_80064D4E));
    for (i = 0; i < TRACK_COUNT; ++i) {
        D_800C5328[i][0] = (s16)(20 + i);
        D_800C532A[i][0] = (s16)(80 + i);
        D_800C5328[i][1] = 89;
        D_800C532A[i][1] = 91;
    }
    for (i = 0; i < SLOT_COUNT; ++i) {
        D_800A4638.slots[i].address = 733 - 39 * i;
        D_800A4638.slots[i].empty = 1;
        D_800A4638.slots[i].handle = PREVIOUS_HANDLE;
    }
    requested_slot = slot;
    requested_track = 2 + 2 * slot;
    previous_track = requested_track + 1;
    alternate_track = previous_track + 1;
    expected_address = D_800A4638.slots[slot].address;
    expected_header = INPUT_HEADER;
    D_800A4638.slots[slot].track = (s16)previous_track;
    D_800A4638.slots[slot].other_track = (s16)alternate_track;
    D_80064D49[requested_track][0] = (u8)slot;
    D_800A4EF6 = 125;
    D_800A4638.handle = PREVIOUS_HANDLE;
    opened_handle = -23;
    poll_result = first_result = second_result = 0;
    mutate_release = mutate_phases = redirect_close = 0;
    close_argument = 0;
    call_count = 0;
    calls[0] = 0;
}

static void open_failure_with_live_references(int slot)
{
    reset(slot);
    D_800A4638.slots[slot].other_type = 35;
    D_80064D4A[requested_track][0] = 21;
    D_800A4638.slots[slot].empty = 0;
    mutate_release = 1;
    expected_address = -14052;
    opened_handle = -1;
    assert(func_8002FF0C(requested_track, INPUT_HEADER) == 1);
    assert(strcmp(calls, "ro") == 0);
    assert(D_800A4638.phase == 0 && D_800A4638.handle == -1);
    assert(D_800A4638.slots[slot].handle == -1 && D_800A4638.slots[slot].empty == 1);
    assert(D_800A4638.slots[(slot + 1) % SLOT_COUNT].handle == PREVIOUS_HANDLE);
    for (int i = 0; i < TRACK_COUNT; ++i) {
        assert(D_800C5328[i][0] == (i == previous_track ? -1 : 20 + i));
        assert(D_800C532A[i][0] == (i == alternate_track ? -1 : 80 + i));
        assert(D_800C5328[i][1] == 89 && D_800C532A[i][1] == 91);
    }
}

static void poll_failure_preserves_matching_reference(int slot)
{
    reset(slot);
    D_800A4638.slots[slot].other_type = 21;
    D_80064D4A[requested_track][0] = 21;
    poll_result = 131071; /* Only the low half is tested for -1. */
    assert(func_8002FF0C(requested_track, INPUT_HEADER) == 1);
    assert(strcmp(calls, "op") == 0);
    assert(D_800A4638.phase == 0 && D_800A4638.handle == opened_handle);
    assert(D_800A4638.slots[slot].other_type == 21);
    assert(D_800A4638.slots[slot].other_track == alternate_track);
    assert(D_800C532A[alternate_track][0] == 80 + alternate_track);
    assert(D_800C5328[previous_track][0] == 20 + previous_track);
    assert(D_800A4EF6 == 125);
}

static void resume_and_release(int slot)
{
    reset(slot);
    mutate_phases = redirect_close = 1;
    second_result = -3;
    poll_result = -65536; /* A nonzero high half alone does not signal failure. */
    D_80064D4A[requested_track][0] = 63;
    assert(func_8002FF0C(requested_track, INPUT_HEADER) == 0);
    assert(strcmp(calls, "opa") == 0 && D_800A4638.phase == 2);
    assert(D_800A4638.slots[slot].other_track == alternate_track);
    assert(D_800C532A[alternate_track][0] == 80 + alternate_track);
    assert(func_8002FF0C(0, UPDATED_HEADER) == 1);
    assert(strcmp(calls, "opabc") == 0 && close_argument == -299);
    assert(D_800A4638.handle == -1);
    assert(D_800A4638.slots[slot].handle == opened_handle);
    assert(D_800A4638.slots[(slot + 1) % SLOT_COUNT].handle == -1);
    assert(D_800A4638.track == requested_track && D_800A4638.header == INPUT_HEADER);
}

static void resume_existing_phase(int slot)
{
    reset(slot);
    D_800A4638.slot = (s16)slot;
    D_800A4638.phase = 2;
    assert(func_8002FF0C(0, UPDATED_HEADER) == 0);
    assert(strcmp(calls, "b") == 0 && D_800A4638.slots[slot].handle == PREVIOUS_HANDLE);
    D_800A4638.phase = 1;
    first_result = 65536; /* Phase completion tests the whole word. */
    assert(func_8002FF0C(0, UPDATED_HEADER) == 1);
    assert(strcmp(calls, "bac") == 0 && close_argument == PREVIOUS_HANDLE);
    assert(D_800A4638.handle == -1 && D_800A4638.slots[slot].handle == -1);
}

int main(void)
{
    for (int slot = 0; slot < SLOT_COUNT; ++slot) {
        open_failure_with_live_references(slot);
        poll_failure_preserves_matching_reference(slot);
        resume_and_release(slot);
        resume_existing_phase(slot);
    }
    reset(0);
    for (int phase = -3; phase <= 3; phase += 6) {
        unsigned char before[sizeof(D_800A4638)];
        D_800A4638.phase = (s16)phase;
        memcpy(before, &D_800A4638, sizeof(before));
        assert(func_8002FF0C(requested_track, INPUT_HEADER) == 0);
        assert(call_count == 0 && memcmp(before, &D_800A4638, sizeof(before)) == 0);
    }
    return 0;
}
