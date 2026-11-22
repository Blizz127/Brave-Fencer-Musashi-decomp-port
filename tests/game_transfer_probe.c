#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../src/main/8001a338.c"

Stream D_800AE6F0;
Byte D_80078E40[12];
PartHeader D_80079A70;
_Alignas(TileRect) Byte D_8007A280[2048];
Word D_80072C80;
Word D_800AE7AC;

static Byte destination[2400], payload[2200];
static Word observed_sector;
static unsigned position_calls, discard_calls, start_calls;
static TrackRequest requests[2];
static int stream_base;
static int cd_result, image_result, decode_result;
static unsigned decode_calls;
static Byte tile_map[64];
static unsigned draws;
static const void *expected_rect, *expected_pixels;

typedef struct {
    void *destination;
    int words;
    const void *contents;
} ExpectedRead;
static ExpectedRead reads[5];
static unsigned queued, consumed;

static Word address(const void *pointer)
{
    assert((uintptr_t)pointer <= UINT32_MAX);
    return (Word)(uintptr_t)pointer;
}

static void begin(Word mode)
{
    memset(&D_800AE6F0, 0, sizeof D_800AE6F0);
    D_800AE6F0.mode = mode;
    D_800AE6F0.result = 19;
    observed_sector = D_800AE7AC = 45;
    queued = consumed = position_calls = discard_calls = start_calls = 0;
    stream_base = cd_result = image_result = decode_result = 0;
    draws = decode_calls = 0;
    expected_rect = expected_pixels = NULL;
}

static void expect_read(void *target, int words, const void *contents)
{
    assert(queued < sizeof reads / sizeof reads[0]);
    reads[queued++] = (ExpectedRead){target, words, contents};
}

static void expect_position(void)
{
    expect_read(D_80078E40, 3, NULL);
}

int func_80043994(Word data, int words)
{
    assert(consumed < queued);
    ExpectedRead read = reads[consumed++];
    assert(data == address(read.destination) && words == read.words);
    if (read.contents) memcpy((void *)(uintptr_t)data, read.contents, (size_t)words * 4);
    return cd_result;
}

int func_80043B1C(const void *position)
{
    assert(position == D_80078E40 && consumed == 1);
    ++position_calls;
    return (int)observed_sector;
}

void func_8002FDC8(void) { ++discard_calls; }

int func_8002FB08(int track)
{
    assert(start_calls < 2 && track == requests[start_calls].track);
    if (++start_calls == 1) {
        ++D_800AE6F0.track_index;
        return 0;
    }
    return 1;
}

int func_8002FC64(int bytes, void *data)
{
    (void)bytes;
    (void)data;
    assert(0 && "unexpected track submission");
    return 0;
}

int func_800599B8(TileRect *area, Byte *pixels)
{
    assert(area == expected_rect && pixels == expected_pixels && draws == 0);
    ++draws;
    return image_result;
}

void func_80018714(int base) { stream_base = base; }

int func_80018730(Byte *buffer)
{
    assert(buffer == (Byte *)&D_80079A70);
    ++decode_calls;
    return decode_result;
}

static void notify(Byte event)
{
    func_8001A338(event);
    assert(consumed == queued);
    assert(position_calls == (unsigned)(event == 1));
    queued = consumed = position_calls = 0;
}

static void raw_sectors(void)
{
    Stream *s = &D_800AE6F0;
    begin(3);
    for (unsigned i = 0; i < sizeof payload; ++i)
        payload[i] = (Byte)(i * 13u + (i >> 4) + 9u);
    memset(destination, 0xa5, sizeof destination);
    s->destination = address(destination);
    s->remaining = 523;
    s->last_part = 1;
    observed_sector = D_800AE7AC = UINT32_MAX;
    expect_position();
    expect_read(destination, 512, payload);
    notify(1);
    assert(s->mode == 3 && s->result == 19 && D_800AE7AC == 0);
    assert(s->remaining == 11 && s->destination == address(destination + 2048));
    assert(memcmp(destination, payload, 2048) == 0 && destination[2048] == 0xa5);
    observed_sector = 0;
    expect_position();
    expect_read(destination + 2048, 11, payload + 2048);
    notify(1);
    assert(s->mode == 2 && s->result == 1 && D_800AE7AC == 1);
    assert(memcmp(destination, payload, 523 * 4) == 0);
    for (size_t i = 523 * 4; i < sizeof destination; ++i)
        assert(destination[i] == 0xa5);
}

static void errors_and_waits(void)
{
    Stream *s = &D_800AE6F0;
    for (Word mode = 3; mode <= 4; ++mode) {
        begin(mode);
        ++observed_sector;
        expect_position();
        notify(1);
        assert(s->result == 2 && s->mode == mode && D_800AE7AC == 45);
        assert(discard_calls == (unsigned)(mode == 4));
        begin(mode);
        notify(5);
        assert(s->result == 2 && D_800AE7AC == 45);
        assert(discard_calls == (unsigned)(mode == 4));
    }
    begin(4);
    notify(0);
    assert(s->result == 19 && discard_calls == 0 && D_800AE7AC == 45);
    begin(6);
    s->remaining = 513;
    s->last_part = 1;
    expect_position();
    notify(1);
    assert(s->remaining == 1 && s->mode == 6 && s->result == 19);
    ++observed_sector;
    expect_position();
    notify(1);
    assert(s->remaining == (Word)-511 && s->mode == 2 && s->result == 1);
    begin(7);
    expect_position();
    notify(1);
    assert(s->result == 2 && s->mode == 7 && D_800AE7AC == 46);
}

/* The four-word PAC header is modeled independently from the input function. */
static void header(Byte kind, Byte last, Word size, int valid)
{
    Byte bytes[16] = {'P', 'A', 'C', 0};
    if (!valid) bytes[3] = 'X';
    bytes[4] = kind;
    bytes[5] = last;
    bytes[8] = 1; /* One rectangle in the synthetic rectangle-list case. */
    for (unsigned i = 0; i < 4; ++i) bytes[12 + i] = (Byte)(size >> (8u * i));
    expect_position();
    expect_read(&D_80079A70, 4, bytes);
    if (valid && (kind == 0 || kind == 5)) expect_read(D_800AE6F0.tile_map, 16, tile_map);
    if (valid && kind == 6) expect_read(D_8007A280, 508, NULL);
    notify(1);
}

static void part_decisions(void)
{
    Stream *s = &D_800AE6F0;
    begin(0);
    header(1, 1, 2053, 0);
    assert(s->result == 2 && !s->last_part && !s->mode && D_800AE7AC == 46);
    begin(0);
    s->initial_destination = address(destination);
    header(1, 1, 2053, 1);
    assert(s->mode == 3 && s->remaining == 2 && s->destination == address(destination));
    assert(s->last_part == 1 && s->result == 19);
    begin(0);
    D_80072C80 = address(payload);
    header(7, 0, 2061, 1);
    assert(s->mode == 3 && s->destination == address(payload) && s->remaining == 4);
    begin(0);
    s->request_size = -1;
    header(7, 1, 2061, 1);
    assert(s->mode == 2 && s->result == 1 && s->destination == 0);
    begin(0);
    s->skip_tracks = 1;
    s->tracks = 0; /* A skipped track must not dereference the track table. */
    s->destination = 17;
    s->remaining = 31;
    header(2, 0, 2055, 1);
    assert(s->mode == 2 && s->result == 1 && s->last_part == 0);
    assert(s->destination == 17 && s->remaining == 31);
    begin(0);
    s->skip_tracks = 1;
    header(3, 0, 2055, 1);
    assert(s->mode == 2 && s->result == 1 && start_calls == 0);
    begin(0);
    s->initial_destination = address(destination);
    header(4, 0, 2049, 1);
    assert(s->mode == 5 && s->remaining == 1 && (Word)stream_base == address(destination));
    begin(0);
    header(8, 0, 2057, 1);
    assert(s->mode == 6 && s->remaining == 3 && !s->destination);
    begin(0);
    header(17, 1, 2048, 1);
    assert(s->mode == 0 && s->last_part == 1 && s->result == 19);
}

static void tile_parts(void)
{
    Stream *s = &D_800AE6F0;
    begin(0);
    memset(tile_map, 0, sizeof tile_map);
    tile_map[4] = 2;
    header(0, 1, 2048, 1);
    assert(s->mode == 1 && s->map_index == 4 && s->bit == 2);
    assert(s->rect.x == 32 && s->rect.y == 32 && s->rect.w == 32 && s->rect.h == 32);
    ++observed_sector;
    expect_position();
    expect_read(&D_80079A70, 512, NULL);
    expected_rect = &s->rect;
    expected_pixels = &D_80079A70;
    notify(1);
    assert(draws == 1 && s->buffer_index == 1 && s->mode == 2 && s->result == 1);

    begin(0);
    memset(tile_map, 0, sizeof tile_map);
    header(5, 0, 2048, 1);
    assert(s->map_index == 64 && s->mode == 1 && draws == 0);

    begin(0);
    memset(D_8007A280, 0, sizeof D_8007A280);
    expected_rect = D_8007A280;
    expected_pixels = D_8007A280 + 144;
    header(6, 1, 2048, 1);
    assert(draws == 1 && s->rect_cursor == address(D_8007A280 + 8));
    assert(s->mode == 2 && s->result == 1);
}

static void track_alias_and_wrap(void)
{
    Stream *s = &D_800AE6F0;
    begin(0);
    s->tracks = address(&s->remaining) - sizeof(Word);
    s->track_index = 0x20000000u;
    s->remaining = address(destination);
    header(2, 0, 2053, 1);
    assert(s->mode == 3 && s->destination == address(destination) && s->remaining == 2);

    begin(0);
    requests[0].track = 91;
    requests[1].track = 23;
    s->tracks = address(requests);
    s->track_index = 0x20000000u;
    header(3, 0, 2061, 1);
    assert(start_calls == 2 && s->mode == 4 && s->remaining == 13);

    begin(3);
    s->remaining = 512;
    s->destination = address(destination);
    expect_position();
    expect_read(destination, 512, NULL);
    notify(1);
    assert(s->mode == 0 && s->remaining == 512 && s->destination == address(destination));

    begin(3);
    s->destination = UINT32_MAX - 1023u;
    s->remaining = 513;
    expect_position();
    /* The mock records the address; no host storage exists at this guest location. */
    expect_read((void *)(uintptr_t)s->destination, 512, NULL);
    notify(1);
    assert(s->destination == 1024 && s->remaining == 1 && s->mode == 3);
}

static void provider_returns(void)
{
    Stream *s = &D_800AE6F0;
    const int results[] = {-7, 0, 19};
    for (unsigned i = 0; i < sizeof results / sizeof results[0]; ++i) {
        /* CD reads and tile draws return status words that this callback ignores. */
        begin(0);
        cd_result = image_result = results[i];
        expected_rect = D_8007A280;
        expected_pixels = D_8007A280 + 144;
        header(6, 1, 2048, 1);
        assert(draws == 1 && s->mode == 2 && s->result == 1);

        begin(5);
        decode_result = results[i];
        s->last_part = 1;
        expect_position();
        expect_read(&D_80079A70, 512, NULL);
        notify(1);
        assert(decode_calls == 1);
        if (decode_result == 0) assert(s->mode == 2 && s->result == 1);
        else assert(s->mode == 5 && s->result == 19);
    }

    begin(0);
    s->initial_destination = UINT32_C(0xf1234567);
    header(4, 0, 2050, 1);
    assert(stream_base < 0 && (Word)stream_base == s->initial_destination);
}

int main(void)
{
    raw_sectors();
    errors_and_waits();
    part_decisions();
    tile_parts();
    track_alias_and_wrap();
    provider_returns();
    return 0;
}
