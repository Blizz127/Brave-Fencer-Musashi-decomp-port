#include "card_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char primary[] = "SYNTHETIC-ENTRY-0001";
static uint8_t fixture[CARD_IMAGE_SIZE], current[CARD_IMAGE_SIZE];
static uint8_t expected[CARD_IMAGE_SIZE], golden[3 * CARD_BLOCK_SIZE];
static uint8_t output[CARD_IMAGE_SIZE], payload[CARD_IMAGE_SIZE];
static CardImage card = { current, sizeof(current) };
static unsigned checks, scenarios;

#define CHECK(expression) do {                                             \
    if (!(expression)) {                                                   \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expression);   \
        exit(EXIT_FAILURE);                                                \
    }                                                                      \
    ++checks;                                                              \
} while (0)

static void load(const char *path, uint8_t *data, size_t size)
{
    FILE *file = fopen(path, "rb");
    CHECK(file != NULL);
    CHECK(fread(data, 1, size, file) == size);
    CHECK(fgetc(file) == EOF && !ferror(file));
    CHECK(fclose(file) == 0);
}

static void reset(void)
{
    memcpy(current, fixture, sizeof(current));
    memcpy(expected, fixture, sizeof(expected));
}

static uint8_t *sector(uint8_t *image, unsigned number)
{
    return image + number * 128u;
}

/* The fixture declarations specify five diverted logical sectors. */
static unsigned physical(unsigned logical)
{
    static const unsigned diverted[] = {0, 3, 14 * 64, 64, 1};
    unsigned i;
    for (i = 0; i < sizeof(diverted) / sizeof(diverted[0]); ++i) {
        if (logical == diverted[i])
            return 36u + i;
    }
    return logical;
}

static void seal(uint8_t *record)
{
    unsigned i;
    uint8_t value = 0;
    for (i = 0; i < 127; ++i)
        value ^= record[i];
    record[127] = value;
}

static void put16(uint8_t *addr, unsigned value)
{
    addr[0] = (uint8_t)(value % 256u);
    addr[1] = (uint8_t)(value / 256u);
}

static void put32(uint8_t *addr, uint32_t value)
{
    unsigned i;
    for (i = 0; i < 4; ++i) {
        addr[i] = (uint8_t)(value % 256u);
        value /= 256u;
    }
}

static void test_lookup(void)
{
    CardEntry files[CARD_MAX_FILES];
    CardEntry entry;
    size_t count = 100;
    reset();
    CHECK(card_validate(card) == CARD_OK);
    CHECK(card_list(card, files, &count) == CARD_OK);
    CHECK(count == 2);
    CHECK(strcmp(files[0].name, "SECOND") == 0 && files[0].first_sector == 128);
    CHECK(strcmp(files[1].name, primary) == 0 && files[1].first_sector == 192);
    CHECK(files[0].size == CARD_BLOCK_SIZE && files[1].size == sizeof(golden));
    CHECK(card_open(card, primary, &entry) == CARD_OK);
    CHECK(strcmp(entry.name, primary) == 0 && entry.name[20] == '\0');
    CHECK(entry.first_sector == 192 && entry.size == sizeof(golden));
    CHECK(card_open(card, "second", &entry) == CARD_NOT_FOUND);
    CHECK(card_open(card, "SYNTHETIC-ENTRY-0002", &entry) == CARD_NOT_FOUND);
    CHECK(memcmp(current, fixture, sizeof(current)) == 0);
    ++scenarios;
}

static void test_header_signature(void)
{
    int recognized = 99;
    CardImage shortened = {current, sizeof(current) - 1};
    reset();
    CHECK(current[0] != 'M');
    CHECK(card_header_signature(card, &recognized) == CARD_OK && recognized == 1);
    CHECK(memcmp(current, fixture, sizeof(current)) == 0);
    /* Directory integrity and the non-identifier bytes of sector 0 are
       distinct questions from whether a readable header has the card ID. */
    sector(current, physical(3))[127] ^= 1;
    sector(current, physical(0))[127] ^= 1;
    CHECK(card_validate(card) == CARD_INVALID);
    CHECK(card_header_signature(card, &recognized) == CARD_OK && recognized == 1);
    current[0] = 'M';
    current[1] = 'C';
    sector(current, physical(0))[1] = 'X';
    CHECK(card_header_signature(card, &recognized) == CARD_OK && recognized == 0);
    sector(current, 16)[127] ^= 1;
    recognized = 99;
    CHECK(card_header_signature(card, &recognized) == CARD_INVALID && recognized == 99);
    CHECK(card_header_signature(shortened, &recognized) == CARD_INVALID && recognized == 99);
    CHECK(card_header_signature(card, NULL) == CARD_ARGUMENT);
    ++scenarios;
}

static void test_blank_image(void)
{
    CardImage short_image = {current, sizeof(current) - 1u};
    CardImage missing_buffer = {NULL, sizeof(current)};
    CardEntry entries[CARD_MAX_FILES];
    size_t count = 44;
    int recognized = 17;
    memset(current, 0, sizeof(current));
    memset(expected, 0, sizeof(expected));
    CHECK(card_is_blank(card));
    CHECK(!card_is_blank(short_image));
    CHECK(!card_is_blank(missing_buffer));
    CHECK(card_header_signature(card, &recognized) == CARD_OK && recognized == 0);
    CHECK(card_validate(card) == CARD_INVALID);
    CHECK(card_list(card, entries, &count) == CARD_INVALID && count == 44);
    CHECK(card_create(card, "ONE", 1) == CARD_INVALID);
    CHECK(card_write(card, "ONE", 0, "X", 1) == CARD_INVALID);
    CHECK(card_delete(card, "ONE") == CARD_INVALID);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);

    current[sizeof(current) - 1u] = 1;
    recognized = 17;
    CHECK(!card_is_blank(card));
    CHECK(card_header_signature(card, &recognized) == CARD_INVALID && recognized == 17);
    CHECK(card_validate(card) == CARD_INVALID);
    current[sizeof(current) - 1u] = 0;
    current[0] = 'M';
    current[1] = 'C';
    CHECK(!card_is_blank(card));
    CHECK(card_header_signature(card, &recognized) == CARD_INVALID && recognized == 17);
    CHECK(card_validate(card) == CARD_INVALID);

    reset();
    sector(current, 16)[127] ^= 1;
    memcpy(expected, current, sizeof(current));
    CHECK(!card_is_blank(card));
    CHECK(card_header_signature(card, &recognized) == CARD_INVALID && recognized == 17);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    reset();
    sector(current, physical(0))[0] = 'X';
    seal(sector(current, physical(0)));
    CHECK(card_header_signature(card, &recognized) == CARD_OK && recognized == 0);
    CHECK(!card_is_blank(card));
    CHECK(card_validate(card) == CARD_INVALID);

    memset(current, 0, sizeof(current));
    CHECK(card_format(card) == CARD_OK);
    CHECK(!card_is_blank(card));
    CHECK(card_header_signature(card, &recognized) == CARD_OK && recognized == 1);
    CHECK(card_validate(card) == CARD_OK);
    ++scenarios;
}

static void test_read(void)
{
    size_t got;
    reset();
    memset(output, 0xa4, sizeof(output));
    CHECK(card_read(card, primary, 0, output, sizeof(output), &got) == CARD_OK);
    CHECK(got == sizeof(golden) && memcmp(output, golden, got) == 0);
    CHECK(output[sizeof(golden)] == 0xa4);
    memset(output, 0xa4, 128);
    CHECK(card_read(card, primary, sizeof(golden) - 2, output, SIZE_MAX, &got) == CARD_OK);
    CHECK(got == 2 && memcmp(output, golden + sizeof(golden) - 2, got) == 0);
    CHECK(output[2] == 0xa4);
    CHECK(card_read(card, primary, sizeof(golden), output, 128, &got) == CARD_OK);
    CHECK(got == 0);
    CHECK(card_read(card, primary, sizeof(golden), NULL, 0, &got) == CARD_OK);
    CHECK(got == 0);
    CHECK(card_read(card, primary, 0, NULL, 1, &got) == CARD_ARGUMENT);
    ++scenarios;
}

static void test_short_write(void)
{
    size_t got;
    reset();
    expected[3u * CARD_BLOCK_SIZE + 17] = 0x4b;
    CHECK(card_write(card, primary, 17, "K", 1) == CARD_OK);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    CHECK(card_write(card, primary, sizeof(golden), NULL, 0) == CARD_OK);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    CHECK(card_read(card, primary, 16, output, 3, &got) == CARD_OK);
    CHECK(got == 3 && output[0] == golden[16] && output[1] == 0x4b &&
          output[2] == golden[18]);
    ++scenarios;
}

static void test_fragmented_write(void)
{
    static const unsigned ordered_blocks[] = {3, 14, 5};
    size_t offset = CARD_BLOCK_SIZE - 31;
    size_t length = CARD_BLOCK_SIZE + 200;
    size_t i, got;
    reset();
    for (i = 0; i < length; ++i) {
        size_t logical_offset = offset + i;
        unsigned in_block = (unsigned)(logical_offset % CARD_BLOCK_SIZE);
        unsigned logical_sector = ordered_blocks[logical_offset / CARD_BLOCK_SIZE] * 64
                                + in_block / 128;
        expected[128 * physical(logical_sector) + in_block % 128] = payload[i];
    }
    CHECK(card_write(card, primary, offset, payload, length) == CARD_OK);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    CHECK(card_read(card, primary, offset, output, length, &got) == CARD_OK);
    CHECK(got == length && memcmp(output, payload, got) == 0);
    CHECK(card_read(card, "SECOND", 0, output, CARD_BLOCK_SIZE, &got) == CARD_OK);
    CHECK(got == CARD_BLOCK_SIZE && memcmp(output, fixture + 2 * CARD_BLOCK_SIZE, got) == 0);
    ++scenarios;
}

static void test_failed_edits(void)
{
    size_t got = 22;
    CardEntry entry;
    CardImage short_image = { current, sizeof(current) - 1 };
    CardImage empty = { NULL, sizeof(current) };
    reset();
    CHECK(card_validate(short_image) == CARD_INVALID);
    CHECK(card_validate(empty) == CARD_ARGUMENT);
    CHECK(card_format(short_image) == CARD_ARGUMENT);
    CHECK(card_format(empty) == CARD_ARGUMENT);
    CHECK(card_open(card, NULL, &entry) == CARD_ARGUMENT);
    CHECK(card_open(card, "", &entry) == CARD_ARGUMENT);
    CHECK(card_create(card, "SYNTHETIC-ENTRY-00011", 1) == CARD_ARGUMENT);
    CHECK(card_create(card, "NEW", 0) == CARD_RANGE);
    CHECK(card_create(card, "NEW", SIZE_MAX) == CARD_RANGE);
    CHECK(card_create(card, primary, 1) == CARD_EXISTS);
    CHECK(card_create(card, "NEW", 12) == CARD_NO_SPACE);
    CHECK(card_write(card, primary, sizeof(golden) - 2, "four", 4) == CARD_RANGE);
    CHECK(card_write(card, primary, SIZE_MAX, NULL, 0) == CARD_RANGE);
    CHECK(card_write(card, primary, 2, payload, SIZE_MAX) == CARD_RANGE);
    CHECK(card_write(card, primary, 2, NULL, 1) == CARD_ARGUMENT);
    CHECK(card_write(card, "ABSENT", 0, "x", 1) == CARD_NOT_FOUND);
    CHECK(card_delete(card, "ABSENT") == CARD_NOT_FOUND);
    CHECK(card_read(card, primary, SIZE_MAX, output, 0, &got) == CARD_RANGE);
    CHECK(got == 22);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    ++scenarios;
}

static void test_delete_and_reuse(void)
{
    static const unsigned released[] = {3, 14, 5};
    static const unsigned assigned[] = {1, 3, 4, 5};
    size_t count, got;
    CardEntry files[CARD_MAX_FILES];
    unsigned i, j;
    reset();
    for (i = 0; i < 3; ++i) {
        uint8_t *record = sector(expected, physical(released[i]));
        record[0] = (uint8_t)(161u + i);
        seal(record);
    }
    CHECK(card_delete(card, primary) == CARD_OK);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    CHECK(card_list(card, files, &count) == CARD_OK && count == 1);
    CHECK(card_open(card, primary, &files[0]) == CARD_NOT_FOUND);

    for (i = 0; i < 4; ++i) {
        uint8_t *record = sector(expected, physical(assigned[i]));
        static const unsigned states[] = {81, 82, 82, 83};
        memset(record, 0, 31);
        record[0] = (uint8_t)states[i];
        if (i == 0) {
            put32(record + 4, 4 * CARD_BLOCK_SIZE);
            memcpy(record + 10, "REUSED", 6);
        }
        put16(record + 8, i == 3 ? 65535 : assigned[i + 1] - 1);
        seal(record);
        for (j = 0; j < 64; ++j)
            memset(sector(expected, physical(assigned[i] * 64 + j)), 0, 128);
    }
    CHECK(card_create(card, "REUSED", 4) == CARD_OK);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    CHECK(card_open(card, "REUSED", &files[0]) == CARD_OK);
    CHECK(files[0].size == 4 * CARD_BLOCK_SIZE && files[0].first_sector == 64);
    memset(output, 2, sizeof(output));
    CHECK(card_read(card, "REUSED", 0, output, sizeof(output), &got) == CARD_OK);
    CHECK(got == 4 * CARD_BLOCK_SIZE);
    CHECK(output[got] == 2);
    for (i = 0; i < got; ++i) {
        if (output[i] != 0)
            break;
    }
    CHECK(i == got);
    ++scenarios;
}

/* Each defect first passes the lower-level XOR test unless XOR itself is under test. */
enum Broken {
    HEADER_XOR, HEADER_MAGIC, FREE_XOR, ALLOCATED_XOR, DEFECT_XOR, DUPLICATE_DEFECT,
    DEFECT_TO_TABLE, DEFECT_TO_POOL, DEFECT_OUT_OF_RANGE, INVALID_STATE, UPPER_STATE_BITS,
    EMPTY_NAME, NO_TERMINATOR, DUPLICATE_NAME, ZERO_SIZE, UNALIGNED_SIZE, OVERSIZE,
    RING, ENDS_TOO_SOON, OUT_OF_RANGE_LINK, TAIL_LINK, CLAIMED_BY_TWO, ORPHAN,
    TAIL_IN_MIDDLE, MIDDLE_AT_END, BROKEN_COUNT
};

static void damage(enum Broken issue)
{
    uint8_t *head = sector(current, physical(3));
    uint8_t *middle = sector(current, physical(14));
    uint8_t *tail = sector(current, physical(5));
    uint8_t *other = sector(current, physical(2));
    uint8_t *unused = sector(current, physical(9));
    uint8_t *header = sector(current, physical(0));
    uint8_t *bad = sector(current, 16);
    switch (issue) {
    case HEADER_XOR: header[127] ^= 1; return;
    case FREE_XOR: unused[127] ^= 1; return;
    case ALLOCATED_XOR: middle[127] ^= 1; return;
    case DEFECT_XOR: bad[127] ^= 1; return;
    case HEADER_MAGIC: header[0] = 0; break;
    case DUPLICATE_DEFECT: put32(bad, 3); break;
    case DEFECT_TO_TABLE: put32(bad, 16); break;
    case DEFECT_TO_POOL: put32(bad, 45); break;
    case DEFECT_OUT_OF_RANGE: put32(bad, 1024); break;
    case INVALID_STATE: unused[0] = 19; break;
    case UPPER_STATE_BITS: unused[3] = 7; break;
    case EMPTY_NAME: head[10] = 0; break;
    case NO_TERMINATOR: memset(head + 10, 'A', 21); break;
    case DUPLICATE_NAME: memcpy(head + 10, other + 10, 21); break;
    case ZERO_SIZE: put32(head + 4, 0); break;
    case UNALIGNED_SIZE: put32(head + 4, 17000); break;
    case OVERSIZE: put32(head + 4, UINT32_MAX); break;
    case RING: put16(middle + 8, 2); break;
    case ENDS_TOO_SOON: put16(head + 8, 65535); break;
    case OUT_OF_RANGE_LINK: put16(head + 8, 15); break;
    case TAIL_LINK: put16(tail + 8, 13); break;
    case CLAIMED_BY_TWO: put32(other + 4, 2 * CARD_BLOCK_SIZE); put16(other + 8, 4); break;
    case ORPHAN: unused[0] = 83; break;
    case TAIL_IN_MIDDLE: middle[0] = 83; break;
    case MIDDLE_AT_END: tail[0] = 82; break;
    case BROKEN_COUNT: abort();
    }
    seal(head);
    seal(middle);
    seal(tail);
    seal(other);
    seal(unused);
    seal(header);
    seal(bad);
}

static void test_corruption(void)
{
    enum Broken issue;
    for (issue = HEADER_XOR; issue < BROKEN_COUNT; ++issue) {
        CardEntry files[CARD_MAX_FILES];
        size_t count = 911, got = 722;
        reset();
        damage(issue);
        memcpy(expected, current, sizeof(expected));
        CHECK(card_validate(card) == CARD_INVALID);
        CHECK(card_list(card, files, &count) == CARD_INVALID);
        CHECK(count == 911);
        CHECK(card_open(card, primary, &files[0]) == CARD_INVALID);
        CHECK(card_read(card, primary, 0, output, 3, &got) == CARD_INVALID);
        CHECK(got == 722);
        CHECK(card_write(card, primary, 0, "new", 3) == CARD_INVALID);
        CHECK(card_create(card, "ONE", 1) == CARD_INVALID);
        CHECK(card_delete(card, "SECOND") == CARD_INVALID);
        CHECK(memcmp(current, expected, sizeof(current)) == 0);
        ++scenarios;
    }
}

static void test_format_and_capacity(void)
{
    size_t count, got;
    unsigned i;
    CardEntry files[CARD_MAX_FILES];
    memset(current, 0x2e, sizeof(current));
    CHECK(card_format(card) == CARD_OK);
    CHECK(current[0] == 'M' && current[1] == 'C' && current[127] == 14);
    CHECK(memcmp(current, sector(current, 63), 128) == 0);
    for (i = 1; i <= 15; ++i) {
        const uint8_t *p = sector(current, i);
        CHECK(p[0] == 160 && p[8] == 255 && p[9] == 255 && p[127] == 160);
    }
    for (i = 16; i < 36; ++i) {
        const uint8_t *p = sector(current, i);
        CHECK(p[0] == 255 && p[1] == 255 && p[2] == 255 && p[3] == 255 && p[127] == 0);
    }
    CHECK(card_list(card, files, &count) == CARD_OK && count == 0);
    CHECK(card_create(card, primary, 15) == CARD_OK);
    CHECK(card_open(card, primary, &files[0]) == CARD_OK);
    CHECK(files[0].first_sector == 64 && files[0].size == 15 * CARD_BLOCK_SIZE);
    CHECK(card_write(card, primary, 0, payload, files[0].size) == CARD_OK);
    CHECK(memcmp(current + CARD_BLOCK_SIZE, payload, files[0].size) == 0);
    CHECK(card_read(card, primary, 0, output, sizeof(output), &got) == CARD_OK);
    CHECK(got == files[0].size && memcmp(output, payload, got) == 0);
    memcpy(expected, current, sizeof(current));
    CHECK(card_create(card, "NEW", 1) == CARD_NO_SPACE);
    CHECK(memcmp(current, expected, sizeof(current)) == 0);
    CHECK(card_delete(card, primary) == CARD_OK);
    for (i = 0; i < 15; ++i) {
        char name[4];
        CHECK(snprintf(name, sizeof(name), "F%02u", i) == 3);
        CHECK(card_create(card, name, 1) == CARD_OK);
    }
    CHECK(card_list(card, files, &count) == CARD_OK && count == 15);
    CHECK(strcmp(files[14].name, "F14") == 0 && files[14].first_sector == 960);
    ++scenarios;
}

int main(void)
{
    size_t i;
    load("synthetic-card.bin", fixture, sizeof(fixture));
    load("synthetic-file.bin", golden, sizeof(golden));
    for (i = 0; i < sizeof(payload); ++i)
        payload[i] = (uint8_t)((i * 197u + i / 37u) % 251u);
    test_lookup();
    test_header_signature();
    test_blank_image();
    test_read();
    test_short_write();
    test_fragmented_write();
    test_failed_edits();
    test_delete_and_reuse();
    test_corruption();
    test_format_and_capacity();
    printf("%u scenarios, %u checks passed\n", scenarios, checks);
    return 0;
}
