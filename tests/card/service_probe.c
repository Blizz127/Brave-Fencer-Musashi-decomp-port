#include "card_service.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

static const char slot0[] = "card-store/slot0.mcr";
static const char slot1[] = "card-store/slot1.mcr";

#define CHECK(condition) do {                                                    \
    if (!(condition)) {                                                         \
        fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition);          \
        exit(EXIT_FAILURE);                                                     \
    }                                                                           \
} while (0)

static void pattern(unsigned char *buf, size_t length, size_t file_offset, unsigned seed)
{
    for (size_t i = 0; i < length; ++i) {
        size_t pos = file_offset + i;
        buf[i] = (unsigned char)(seed + pos * 29u + (pos >> 6) + (pos >> 11));
    }
}

static int uniform(const unsigned char *buf, size_t length, unsigned char value)
{
    for (size_t i = 0; i < length; ++i) {
        if (buf[i] != value) {
            return 0;
        }
    }
    return 1;
}

/* In a fresh test directory, make the pathname traverse a non-directory.
   This works independently of host privileges and preserves the entire image. */
static void hide_media(void)
{
    CHECK(rename("card-store", "saved-store") == 0);
    FILE *barrier = fopen("card-store", "wb");
    CHECK(barrier != NULL);
    CHECK(fclose(barrier) == 0);
}

static void restore_media(void)
{
    CHECK(remove("card-store") == 0);
    CHECK(rename("saved-store", "card-store") == 0);
}

enum {
    MEMORY_SIZE = 32768, PATH = 33, DATA = 257, DIRECTORY = 23043,
    FILE_BYTES = 16384, SECTOR = 128, BLOCK = 8192
};

typedef struct {
    uint32_t kind;
    uint32_t spec;
} Notice;

typedef struct {
    _Alignas(8) unsigned char memory[MEMORY_SIZE];
    int absent[2];
    unsigned snapshots;
    unsigned commits;
    int refuse_commit;
    int inject_rival;
    int use_cas_card;
    uint32_t revoked_address;
    unsigned attempts;
    size_t accepted;
    int stall_after; /* -1 disables refusal */
    Notice log[16];
} TestHost;

static const char *media_path(const TestHost *h, unsigned slot)
{
    CHECK(slot <= 1);
    if (slot == 1 && h->use_cas_card) {
        return "card-store/cas.mcr";
    }
    return slot ? slot1 : slot0;
}

static uint8_t *map_guest(void *context, uint32_t address, size_t count, int writable)
{
    TestHost *h = context;
    (void)writable;
    if (!address || address > MEMORY_SIZE || count > MEMORY_SIZE - address) {
        return NULL;
    }
    if (h->revoked_address >= address && h->revoked_address - address < count) {
        return NULL;
    }
    return h->memory + address;
}

static CsMedia snapshot(void *context, unsigned slot, uint8_t *image)
{
    TestHost *h = context;
    ++h->snapshots;
    if (slot > 1) {
        return CS_MEDIA_ERROR;
    }
    if (h->absent[slot]) {
        return CS_MEDIA_ABSENT;
    }
    /* A non-directory parent can also yield ENOENT through the Windows CRT. */
#ifdef _WIN32
    DWORD attributes = GetFileAttributesA("card-store");
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return CS_MEDIA_ERROR;
    }
#else
    struct stat directory;
    if (stat("card-store", &directory) != 0 || !S_ISDIR(directory.st_mode)) {
        return CS_MEDIA_ERROR;
    }
#endif
    FILE *f = fopen(media_path(h, slot), "rb");
    if (!f) {
        return errno == ENOENT ? CS_MEDIA_MISSING : CS_MEDIA_ERROR;
    }
    size_t n = fread(image, 1, CS_IMAGE_BYTES, f);
    int exact = n == CS_IMAGE_BYTES && fgetc(f) == EOF && !ferror(f);
    int closed = fclose(f) == 0;
    return exact && closed ? CS_MEDIA_OK : CS_MEDIA_ERROR;
}

static int commit(void *context, unsigned slot, const uint8_t *expected,
                  const uint8_t *replacement)
{
    TestHost *h = context;
    ++h->commits;
    uint8_t *old = malloc(CS_IMAGE_BYTES);
    CHECK(old != NULL);
    if (h->inject_rival) {
        /* The front-end already took its snapshot. Commit a fixture change
           at the final byte before the host compares against that snapshot. */
        CHECK(slot == 1 && expected && h->use_cas_card);
        h->inject_rival = 0;
        FILE *rival = fopen(media_path(h, slot), "r+b");
        CHECK(rival && fseek(rival, -1L, SEEK_END) == 0);
        int previous = fgetc(rival);
        CHECK(previous != EOF && fseek(rival, -1L, SEEK_END) == 0);
        CHECK(fputc(previous ^ 0x5a, rival) != EOF && fclose(rival) == 0);
    }
    CsMedia old_state = snapshot(context, slot, old);
    int current = expected ? old_state == CS_MEDIA_OK && !memcmp(old, expected, CS_IMAGE_BYTES)
                           : old_state == CS_MEDIA_MISSING;
    free(old);
    if (!current || h->refuse_commit) {
        return 0;
    }

    /* The suite is the only writer to this new per-run directory. */
    const char *temp = "card-store/candidate";
    FILE *f = fopen(temp, "wb");
    if (!f) {
        return 0;
    }
    size_t n = fwrite(replacement, 1, CS_IMAGE_BYTES, f);
    int closed = fclose(f) == 0;
    if (closed && n == CS_IMAGE_BYTES) {
#ifdef _WIN32
        if (MoveFileExA(temp, media_path(h, slot), MOVEFILE_REPLACE_EXISTING)) {
            return 1;
        }
#else
        if (rename(temp, media_path(h, slot)) == 0) {
            return 1;
        }
#endif
    }
    CHECK(remove(temp) == 0);
    return 0;
}

static int event(void *context, uint32_t kind, uint32_t spec)
{
    TestHost *h = context;
    ++h->attempts;
    if (h->stall_after >= 0 && h->accepted == (size_t)h->stall_after) {
        return 0;
    }
    CHECK(h->accepted < sizeof(h->log) / sizeof(*h->log));
    h->log[h->accepted++] = (Notice){ kind, spec };
    return 1;
}

static CardService *new_service(TestHost *h)
{
    h->stall_after = -1;
    CsHost callbacks = { .context = h, .map = map_guest, .snapshot = snapshot,
                         .commit = commit, .event = event };
    CardService *s = cs_create(callbacks);
    CHECK(s);
    cs_init_card(s);
    cs_start_card(s);
    cs_init_files(s);
    return s;
}

static void drain(CardService *s)
{
    unsigned i = 0;
    while (!cs_idle(s) && i++ < 32) {
        cs_pump(s);
    }
    CHECK(cs_idle(s));
}

static void check_event(const TestHost *h, size_t index, uint32_t kind, uint32_t spec)
{
    CHECK(index < h->accepted);
    if (h->log[index].kind != kind || h->log[index].spec != spec) {
        fprintf(stderr, "event %zu got (%08x,%04x), wanted (%08x,%04x)\n",
                index, (unsigned)h->log[index].kind, (unsigned)h->log[index].spec,
                (unsigned)kind, (unsigned)spec);
        exit(EXIT_FAILURE);
    }
}

static void reset_events(TestHost *h)
{
    h->accepted = 0;
    h->attempts = 0;
    h->stall_after = -1;
}

static uint32_t path(TestHost *h, const char *value)
{
    CHECK(strlen(value) + 1 < DATA - PATH);
    memcpy(h->memory + PATH, value, strlen(value) + 1);
    return PATH;
}

static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void verify_directory(CardService *s, TestHost *h)
{
    unsigned char *out = h->memory + DIRECTORY;
    memset(out - 3, 0xcd, CS_DIRENT_BYTES + 6);
    CHECK(cs_first(s, path(h, "bu00:ALFA?*unused"), DIRECTORY) == DIRECTORY);
    unsigned found = 0;
    do {
        CHECK(uniform(out - 3, 3, 0xcd) && uniform(out + CS_DIRENT_BYTES, 3, 0xcd));
        CHECK(le32(out + 20) == 0x50);
        if (!memcmp(out, "ALFA1", 6)) {
            CHECK(!(found & 1) && le32(out + 24) == FILE_BYTES);
            found |= 1;
        } else {
            CHECK(!memcmp(out, "ALFA9", 6));
            CHECK(!(found & 2) && le32(out + 24) == BLOCK);
            found |= 2;
        }
    } while (cs_next(s, DIRECTORY) == DIRECTORY);
    CHECK(found == 3);

    CHECK(cs_first(s, path(h, "bu00:Z?TA"), DIRECTORY) == DIRECTORY);
    CHECK(!memcmp(out, "ZETA", 5));
    CHECK(cs_next(s, DIRECTORY) == 0);
    CHECK(cs_first(s, path(h, "bu00:Z??TA"), DIRECTORY) == 0);
    memset(h->memory + MEMORY_SIZE - 10, 0xdd, 10);
    CHECK(cs_first(s, path(h, "bu00:*"), MEMORY_SIZE - 10) == 0);
    CHECK(uniform(h->memory + MEMORY_SIZE - 10, 10, 0xdd));
}

static void create_files(CardService *s, TestHost *h)
{
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    CHECK(h->commits == 1);
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    CHECK(h->commits == 1);
    int32_t fd = cs_open(s, path(h, "bu00:TEMP"), 1u << 16 | 0x203u);
    CHECK(fd >= 2 && cs_close(s, fd) == fd);
    CHECK(cs_erase(s, path(h, "bu00:TEMP")) == 1);
    CHECK(cs_open(s, path(h, "bu00:TEMP"), 1) < 0);
    fd = cs_open(s, path(h, "bu00:BEFORE-FORMAT"), 1u << 16 | 0x203u);
    CHECK(fd >= 2 && cs_close(s, fd) == fd);
    CHECK(cs_format(s, path(h, "bu00:")) == 1);
    CHECK(cs_open(s, path(h, "bu00:BEFORE-FORMAT"), 1) < 0);

    fd = cs_open(s, path(h, "bu00:ALFA1"), 2u << 16 | 0x203u);
    CHECK(fd >= 2);
    pattern(h->memory + DATA, FILE_BYTES, 0, 77);
    CHECK(cs_write(s, fd, DATA, BLOCK - SECTOR) == BLOCK - SECTOR);
    CHECK(cs_write(s, fd, DATA + BLOCK - SECTOR, 2 * SECTOR) == 2 * SECTOR);
    CHECK(cs_write(s, fd, DATA + BLOCK + SECTOR, BLOCK - SECTOR) == BLOCK - SECTOR);
    CHECK(cs_close(s, fd) == fd);

    const char *others[] = { "bu00:ALFA9", "bu00:ZETA" };
    for (size_t i = 0; i < sizeof(others) / sizeof(*others); ++i) {
        fd = cs_open(s, path(h, others[i]), 1u << 16 | 0x203u);
        CHECK(fd >= 2);
        CHECK(cs_close(s, fd) == fd);
    }
    verify_directory(s, h);
    drain(s);
}

static void verify_contents(CardService *s, TestHost *h)
{
    unsigned char expected[FILE_BYTES];
    pattern(expected, sizeof(expected), 0, 77);
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    CHECK(h->commits == 0);
    int32_t fd = cs_open(s, path(h, "bu00:ALFA1"), 3);
    CHECK(fd >= 2);
    unsigned char *out = h->memory + DATA;
    memset(out - 1, 0xa3, FILE_BYTES + 2);
    CHECK(cs_read(s, fd, DATA, FILE_BYTES) == FILE_BYTES);
    CHECK(!memcmp(out, expected, FILE_BYTES) && out[-1] == 0xa3 && out[FILE_BYTES] == 0xa3);
    CHECK(cs_seek(s, fd, -BLOCK - SECTOR, 1) == BLOCK - SECTOR);
    CHECK(cs_seek(s, fd, -BLOCK, 1) < 0);
    CHECK(cs_seek(s, fd, 0, 1) == BLOCK - SECTOR);
    memset(out, 0xcc, SECTOR * 2);
    CHECK(cs_read(s, fd, DATA, SECTOR * 2) == SECTOR * 2);
    CHECK(!memcmp(out, expected + BLOCK - SECTOR, SECTOR * 2));

    CHECK(cs_seek(s, fd, 0, 0) == 0);
    uint8_t *edge = h->memory + MEMORY_SIZE - SECTOR + 1;
    memset(edge, 0x44, SECTOR - 1);
    CHECK(cs_read(s, fd, MEMORY_SIZE - SECTOR + 1, SECTOR) < 0);
    CHECK(uniform(edge, SECTOR - 1, 0x44));
    CHECK(cs_write(s, fd, UINT32_MAX - SECTOR + 4u, SECTOR) < 0);
    CHECK(cs_read(s, fd, DATA, SECTOR - 1) < 0);
    CHECK(cs_write(s, fd, DATA, SECTOR + 1) < 0);
    CHECK(cs_seek(s, fd, 0, 1) == 0);
    CHECK(cs_read(s, fd, MEMORY_SIZE - SECTOR, SECTOR) == SECTOR);
    CHECK(!memcmp(h->memory + MEMORY_SIZE - SECTOR, expected, SECTOR));

    CHECK(cs_close(s, fd) == fd);
    CHECK(cs_read(s, fd, DATA, SECTOR) < 0);
    CHECK(cs_open(s, path(h, "bu00:GHOST"), 1) == -1);
    CHECK(cs_errno(s) != 0);
    verify_directory(s, h);
    drain(s);
}

static void descriptor_limits(CardService *s, TestHost *h)
{
    CHECK(cs_reserve_fd(s, 2, 1));
    int32_t live = cs_open(s, path(h, "bu00:ALFA1"), 1);
    CHECK(live == 3);
    CHECK(!cs_reserve_fd(s, live, 1));
    CHECK(cs_reserve_fd(s, 2, 0));
    int32_t reused = cs_open(s, path(h, "bu00:ALFA9"), 1);
    CHECK(reused == 2);
    for (int32_t i = 4; i < 16; ++i) {
        CHECK(cs_reserve_fd(s, i, 1));
    }
    CHECK(cs_open(s, path(h, "bu00:ZETA"), 1) < 0);
    CHECK(cs_first(s, path(h, "bu00:*"), DIRECTORY) == 0);
    CHECK(cs_erase(s, path(h, "bu00:ZETA")) == 0);
    CHECK(cs_close(s, reused) == reused);
    CHECK(cs_first(s, path(h, "bu00:ZETA"), DIRECTORY) == DIRECTORY);
    CHECK(cs_close(s, live) == live);
    for (int32_t i = 4; i < 16; ++i) {
        CHECK(cs_reserve_fd(s, i, 0));
    }
}

static void stale_commit(CardService *s, TestHost *h, int inject)
{
    h->use_cas_card = 1;
    uint8_t *before = malloc(CS_IMAGE_BYTES);
    uint8_t *after = malloc(CS_IMAGE_BYTES);
    CHECK(before && after);
    CHECK(snapshot(h, 0, before) == CS_MEDIA_OK);
    if (inject) {
        FILE *seed = fopen(media_path(h, 1), "wb");
        CHECK(seed && fwrite(before, 1, CS_IMAGE_BYTES, seed) == CS_IMAGE_BYTES);
        CHECK(fclose(seed) == 0);
        CHECK(cs_prepare(s, 1) == CS_MEDIA_OK);
        int32_t fd = cs_open(s, path(h, "bu10:ALFA1"), 3);
        CHECK(fd >= 2);
        CHECK(cs_seek(s, fd, SECTOR, 0) == SECTOR);
        pattern(h->memory + DATA, SECTOR, 0, 36);
        h->inject_rival = 1;
        CHECK(cs_write(s, fd, DATA, SECTOR) < 0);
        CHECK(!h->inject_rival);
        CHECK(cs_seek(s, fd, 0, 1) == SECTOR);
        CHECK(cs_close(s, fd) == fd);
    }
    before[CS_IMAGE_BYTES - 1] ^= 0x5a;
    CHECK(snapshot(h, 1, after) == CS_MEDIA_OK && !memcmp(before, after, CS_IMAGE_BYTES));
    free(after);
    free(before);
    h->use_cas_card = 0;
}

static void failed_transactions(CardService *s, TestHost *h)
{
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    int32_t fd = cs_open(s, path(h, "bu00:ALFA1"), 3);
    CHECK(fd >= 2);
    CHECK(cs_seek(s, fd, BLOCK - SECTOR, 0) == BLOCK - SECTOR);
    pattern(h->memory + DATA, SECTOR * 2, 0, 134);
    h->refuse_commit = 1;
    unsigned before = h->commits;
    CHECK(cs_write(s, fd, DATA, SECTOR * 2) == -1);
    CHECK(h->commits == before + 1);
    CHECK(cs_fd_error(s, fd) != 0);
    CHECK(cs_seek(s, fd, 0, 1) == BLOCK - SECTOR);
    CHECK(cs_close(s, fd) == fd);
    CHECK(cs_open(s, path(h, "bu00:GHOST"), 1u << 16 | 0x203u) == -1);
    CHECK(cs_erase(s, path(h, "bu00:ALFA1")) == 0);
    CHECK(cs_format(s, path(h, "bu00:")) == 0);
    CHECK(h->commits == before + 4);
    h->refuse_commit = 0;

    hide_media();
    CHECK(cs_prepare(s, 0) == CS_MEDIA_ERROR);
    restore_media();
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    verify_directory(s, h);
    CHECK(cs_open(s, path(h, "bu00:GHOST"), 1) < 0);

    fd = cs_open(s, path(h, "bu00:ALFA1"), 1);
    CHECK(fd >= 2);
    CHECK(cs_seek(s, fd, BLOCK - SECTOR, 0) == BLOCK - SECTOR);
    unsigned char old[SECTOR * 2];
    pattern(old, sizeof(old), BLOCK - SECTOR, 77);
    CHECK(cs_read(s, fd, DATA, sizeof(old)) == (int)sizeof(old));
    CHECK(!memcmp(old, h->memory + DATA, sizeof(old)));
    CHECK(cs_close(s, fd) == fd);
    drain(s);
}

static void expect_info(CardService *s, TestHost *h, uint32_t hw, uint32_t sw,
                        int status)
{
    reset_events(h);
    CHECK(cs_card_info(s, 0x10) == 1);
    CHECK(cs_card_status(s, 1) == 8);
    CHECK(h->attempts == 0);
    drain(s);
    CHECK(h->accepted == 2);
    check_event(h, 0, CS_EVENT_HW, hw);
    check_event(h, 1, CS_EVENT_SW, sw);
    CHECK(cs_card_chan(s) == 0x10);
    CHECK(cs_card_status(s, 1) == status);
    CHECK(cs_card_wait(s, 1) == status);
}

static void raw_errors(CardService *s, TestHost *h)
{
    CHECK(cs_card_status(s, 0) == 1 && cs_card_status(s, 1) == 1);
    h->absent[1] = 1;
    CHECK(cs_prepare(s, 1) == CS_MEDIA_ABSENT);
    expect_info(s, h, CS_TIMEOUT, CS_TIMEOUT, 0x11);
    h->absent[1] = 0;

    hide_media();
    CHECK(cs_prepare(s, 1) == CS_MEDIA_ERROR);
    expect_info(s, h, CS_ERROR, CS_ERROR, 0x21);
    restore_media();
    /* A partial image must survive a mount/prepare attempt byte for byte. */
    FILE *partial = fopen(slot1, "wb");
    CHECK(partial && fwrite("short", 1, 5, partial) == 5 && fclose(partial) == 0);
    unsigned prior_commits = h->commits;
    CHECK(cs_prepare(s, 1) == CS_MEDIA_ERROR && h->commits == prior_commits);
    expect_info(s, h, CS_ERROR, CS_ERROR, 0x21);
    char recovered[6];
    partial = fopen(slot1, "rb");
    CHECK(partial && fread(recovered, 1, sizeof(recovered), partial) == 5);
    CHECK(!memcmp(recovered, "short", 5) && fclose(partial) == 0);
    CHECK(remove(slot1) == 0);
    CHECK(cs_prepare(s, 1) == CS_MEDIA_OK);
    expect_info(s, h, CS_DONE, CS_DONE, 1);
    cs_media_changed(s, 1);
    expect_info(s, h, CS_CHANGED, CS_CHANGED, 0x21);
    reset_events(h);
    cs_new_card(s);
    CHECK(cs_sector_read(s, 0x10, 1023, DATA) == 1);
    drain(s);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_HW, CS_DONE);
    reset_events(h);
    CHECK(cs_sector_read(s, 0x10, 1023, DATA) == 1);
    drain(s);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_HW, CS_CHANGED);
    reset_events(h);
    CHECK(cs_write_test(s, 0x10) == 1);
    drain(s);
    expect_info(s, h, CS_DONE, CS_DONE, 1);
    cs_media_changed(s, 1);

    reset_events(h);
    CHECK(cs_card_load(s, 0x10) == 1);
    drain(s);
    CHECK(h->accepted == 2);
    check_event(h, 0, CS_EVENT_HW, CS_DONE);
    check_event(h, 1, CS_EVENT_SW, CS_DONE);
    expect_info(s, h, CS_DONE, CS_DONE, 1);

    reset_events(h);
    memset(h->memory + MEMORY_SIZE - SECTOR + 1, 0xb9, SECTOR - 1);
    CHECK(cs_sector_read(s, 0x10, 1023, MEMORY_SIZE - SECTOR + 1) == 0);
    CHECK(cs_sector_write(s, 0x10, 1024, DATA) == 0);
    CHECK(cs_sector_read(s, 0x11, 100, DATA) == 0);
    CHECK(cs_idle(s) && !cs_pump(s) && h->attempts == 0);
    CHECK(uniform(h->memory + MEMORY_SIZE - SECTOR + 1, SECTOR - 1, 0xb9));

    /* Independent save/readback first establishes the old sector bytes. */
    pattern(h->memory + DATA, SECTOR, 0, 63);
    CHECK(cs_sector_write(s, 0x10, 1023, DATA) == 1);
    CHECK(cs_card_status(s, 1) == 4);
    drain(s);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_HW, CS_DONE);

    reset_events(h);
    h->refuse_commit = 1;
    unsigned before = h->commits;
    pattern(h->memory + DATA, SECTOR, 0, 188);
    CHECK(cs_sector_write(s, 0x10, 1023, DATA) == 1);
    CHECK(h->commits == before && h->attempts == 0);
    h->stall_after = 0;
    unsigned result = cs_pump(s);
    CHECK((result & 7) == 5);
    CHECK(h->accepted == 0 && h->commits == before + 1);
    CHECK(cs_card_status(s, 1) == 1 && !cs_idle(s));
    CHECK(cs_card_info(s, 0x10) == 0);
    result = cs_pump(s);
    CHECK((result & 7) == 4 && h->commits == before + 1);
    h->refuse_commit = 0;
    h->stall_after = -1;
    drain(s);
    CHECK(h->accepted == 1 && h->commits == before + 1);
    check_event(h, 0, CS_EVENT_HW, CS_DONE);
    reset_events(h);
    memset(h->memory + DATA - 1, 0x95, SECTOR + 2);
    CHECK(cs_sector_read(s, 0x10, 1023, DATA) == 1);
    h->revoked_address = DATA + SECTOR - 1;
    drain(s);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_HW, CS_ERROR);
    CHECK(uniform(h->memory + DATA - 1, SECTOR + 2, 0x95));
    h->revoked_address = 0;
    expect_info(s, h, CS_PREVIOUS_WRITE, CS_ERROR, 0x21);
    expect_info(s, h, CS_DONE, CS_DONE, 1);

    reset_events(h);
    memset(h->memory + DATA - 1, 0x99, SECTOR + 2);
    CHECK(cs_sector_read(s, 0x10, 1023, DATA) == 1);
    CHECK(cs_card_status(s, 1) == 2);
    CHECK(uniform(h->memory + DATA - 1, SECTOR + 2, 0x99));
    drain(s);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_HW, CS_DONE);
    unsigned char old[SECTOR];
    pattern(old, sizeof(old), 0, 63);
    CHECK(!memcmp(old, h->memory + DATA, SECTOR));
    CHECK(h->memory[DATA - 1] == 0x99 && h->memory[DATA + SECTOR] == 0x99);

    reset_events(h);
    h->stall_after = 1;
    CHECK(cs_card_info(s, 0x10) == 1);
    for (unsigned i = 0; i < 5 && h->attempts < 2; ++i) {
        cs_pump(s);
    }
    CHECK(!cs_idle(s) && h->accepted == 1 && h->attempts == 2);
    check_event(h, 0, CS_EVENT_HW, CS_DONE);
    unsigned snapshots = h->snapshots;
    CHECK((cs_pump(s) & 7) == 4 && h->accepted == 1);
    h->stall_after = -1;
    drain(s);
    CHECK(h->accepted == 2 && h->snapshots == snapshots);
    check_event(h, 1, CS_EVENT_SW, CS_DONE);
    CHECK(cs_pump(s) == 0 && h->attempts == 4);
}

static void replace_pending(CardService *s, TestHost *h)
{
    uint8_t *old_card = malloc(CS_IMAGE_BYTES);
    uint8_t *new_card = malloc(CS_IMAGE_BYTES);
    uint8_t *observed = malloc(CS_IMAGE_BYTES);
    CHECK(old_card && new_card && observed);
    CHECK(snapshot(h, 1, old_card) == CS_MEDIA_OK);
    h->use_cas_card = 1;
    CHECK(snapshot(h, 1, new_card) == CS_MEDIA_OK);
    CHECK(memcmp(old_card, new_card, CS_IMAGE_BYTES) != 0);
    h->use_cas_card = 0;

    reset_events(h);
    pattern(h->memory + DATA, SECTOR, 1023 * SECTOR, 10);
    CHECK(cs_sector_write(s, 0x10, 1023, DATA));
    /* The backing path now points to a different synthetic card. */
    h->use_cas_card = 1;
    cs_media_changed(s, 1);
    drain(s);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_HW, CS_CHANGED);
    CHECK(snapshot(h, 1, observed) == CS_MEDIA_OK && !memcmp(observed, new_card, CS_IMAGE_BYTES));
    h->use_cas_card = 0;
    CHECK(snapshot(h, 1, observed) == CS_MEDIA_OK && !memcmp(observed, old_card, CS_IMAGE_BYTES));
    free(observed);
    free(new_card);
    free(old_card);
}

static void async_files(CardService *s, TestHost *h)
{
    CHECK(cs_prepare(s, 0) == CS_MEDIA_OK);
    int32_t fd = cs_open(s, path(h, "bu00:ALFA1"), 0x8003u);
    CHECK(fd >= 2);
    reset_events(h);
    memset(h->memory + DATA - 1, 0x61, SECTOR + 2);
    CHECK(cs_read(s, fd, DATA, SECTOR) == 0);
    CHECK(uniform(h->memory + DATA - 1, SECTOR + 2, 0x61) && h->attempts == 0);
    drain(s);
    CHECK(h->accepted == 2);
    check_event(h, 0, (uint32_t)fd, CS_DONE);
    check_event(h, 1, CS_EVENT_SW, CS_DONE);
    unsigned char expected[SECTOR];
    pattern(expected, sizeof(expected), 0, 77);
    CHECK(!memcmp(h->memory + DATA, expected, sizeof(expected)));
    CHECK(h->memory[DATA - 1] == 0x61 && h->memory[DATA + SECTOR] == 0x61);

    reset_events(h);
    h->refuse_commit = 1;
    unsigned before = h->commits;
    CHECK(cs_write(s, fd, DATA, SECTOR) == 0 && h->attempts == 0);
    CHECK(h->commits == before);
    drain(s);
    CHECK(h->commits == before + 1);
    CHECK(h->accepted == 1);
    check_event(h, 0, CS_EVENT_SW, CS_ERROR);
    CHECK(cs_fd_error(s, fd) != 0);
    CHECK(cs_seek(s, fd, 0, 1) == SECTOR);
    h->refuse_commit = 0;
    CHECK(cs_close(s, fd) == fd);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    TestHost h = {0};
    CardService *s = new_service(&h);
    if (!strcmp(argv[1], "create")) {
        create_files(s, &h);
    } else if (!strcmp(argv[1], "reopen") || !strcmp(argv[1], "after-failure")) {
        verify_contents(s, &h);
        descriptor_limits(s, &h);
        if (!strcmp(argv[1], "after-failure")) {
            stale_commit(s, &h, 0);
        }
    } else if (!strcmp(argv[1], "failure")) {
        failed_transactions(s, &h);
        stale_commit(s, &h, 1);
    } else if (!strcmp(argv[1], "events")) {
        raw_errors(s, &h);
        replace_pending(s, &h);
        async_files(s, &h);
    } else {
        fprintf(stderr, "unknown test phase %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    CHECK(cs_idle(s));
    cs_destroy(s);
    return EXIT_SUCCESS;
}
