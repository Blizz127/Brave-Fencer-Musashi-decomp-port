/* The real PlayerCard and storage backend, attached to an invented card. */
#undef NDEBUG
#include "player_card.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define PATH_ADDR UINT32_C(0x80078ca0)
#define ROW_ADDR UINT32_C(0x80076044)
#define READ_ADDR UINT32_C(0x80075a40)
#define OTHER_ADDR UINT32_C(0x80010000)
enum { METADATA_SIZE = 640 };
static const char name[] = "bu00:SYNTHETIC-CARD-ROW1";
static const char alternate[] = "bu00:SYNTHETIC-CARD-ROW2";
static MusashiBootMemory ram, other_ram;
static MusashiBiosEvents events, other_events;
static MusashiPlayerCard player;
static uint64_t vblank;

int musashi_boot_execute_bios_event(void *unused, uint32_t callback)
{
    (void)unused;
    (void)callback;
    abort(); /* No guest code or callback record is installed. */
}

static uint8_t *span(uint32_t address, size_t size)
{
    uint8_t *out = musashi_boot_ram_span(&ram, address, size);
    assert(out);
    return out;
}

static void set_path(const char *full_name)
{
    assert(strlen(full_name) == 24);
    memcpy(span(PATH_ADDR, 25), full_name, 25);
}

static void seed_file(const char *full_name, const uint8_t bytes[METADATA_SIZE])
{
    set_path(full_name);
    assert(cs_open(player.service, PATH_ADDR, (1u << 16) | 0x203) == 2);
    assert(cs_seek(player.service, 2, 512, 0) == 512);
    memcpy(span(OTHER_ADDR, METADATA_SIZE), bytes, METADATA_SIZE);
    assert(cs_write(player.service, 2, OTHER_ADDR, METADATA_SIZE) == METADATA_SIZE);
    assert(cs_close(player.service, 2) == 2);
}

static uint64_t select_file(const char *full_name)
{
    set_path(full_name);
    assert(cs_first(player.service, PATH_ADDR, ROW_ADDR) == ROW_ADDR);
    assert(cs_open(player.service, PATH_ADDR, 0x8001) == 2);
    uint64_t id = musashi_player_card_metadata_descriptor(&player, &ram, &events, 2, 0, 0);
    assert(id);
    return id;
}

static void submit(int offset, uint32_t dest, int count)
{
    assert(cs_seek(player.service, 2, offset, 0) == offset);
    assert(cs_read(player.service, 2, dest, count) == 0);
    assert(!cs_idle(player.service));
}

static void pump(void)
{
    musashi_player_card_pulse(&player, ++vblank);
    assert(cs_idle(player.service));
}

static int refused(const MusashiPlayerCard *card, const MusashiBootMemory *memory,
                    const MusashiBiosEvents *table, int32_t fd, uint64_t id)
{
    uint8_t out[METADATA_SIZE + 2], prior[sizeof out];
    for (size_t i = 0; i < sizeof out; ++i) out[i] = (uint8_t)(i + 71);
    memcpy(prior, out, sizeof out);
    if (musashi_player_card_copy_completed_metadata(card, memory, table, fd, id, NULL)) return 0;
    return !musashi_player_card_copy_completed_metadata(card, memory, table, fd, id, out + 1)
        && !memcmp(out, prior, sizeof out);
}

static uint64_t copied(uint64_t id, const uint8_t expected[METADATA_SIZE])
{
    uint8_t out[METADATA_SIZE + 2];
    memset(out, 0x7e, sizeof out);
    uint64_t witness = musashi_player_card_copy_completed_metadata(&player, &ram, &events, 2, id, NULL);
    assert(witness);
    assert(musashi_player_card_copy_completed_metadata(&player, &ram, &events, 2, id, out + 1) == witness);
    assert(!memcmp(out + 1, expected, METADATA_SIZE));
    assert(out[0] == 0x7e && out[sizeof out - 1] == 0x7e);
    return witness;
}

static void verify(const uint8_t expected[METADATA_SIZE])
{
    CardService *s = player.service;
    uint64_t id = select_file(name);
    assert(cs_seek(s, 2, 1152, 0) == 1152);
    assert(refused(&player, &ram, &events, 2, id));
    submit(640, READ_ADDR, 512);
    pump();
    assert(refused(&player, &ram, &events, 2, id));
    submit(512, OTHER_ADDR, METADATA_SIZE);
    pump();
    assert(refused(&player, &ram, &events, 2, id));

    uint8_t *received = span(READ_ADDR, METADATA_SIZE);
    memset(received, 0xee, METADATA_SIZE);
    submit(512, READ_ADDR, METADATA_SIZE);
    assert(refused(&player, &ram, &events, 2, id));
    assert(received[0] == 0xee && received[METADATA_SIZE - 1] == 0xee);
    pump();
    assert(!memcmp(received, expected, METADATA_SIZE));
    uint64_t first_witness = 0;
    /* Mutate the source before the first copy as well as after a valid copy. */
    for (size_t i = 0; i < METADATA_SIZE; ++i) {
        received[i] ^= 1;
        assert(refused(&player, &ram, &events, 2, id));
        received[i] ^= 1;
        if (i == 0) first_witness = copied(id, expected);
    }
    assert(copied(id, expected) == first_witness);
    MusashiPlayerCard before_player;
    MusashiBiosEvents before_events;
    memcpy(&before_player, &player, sizeof player);
    memcpy(&before_events, &events, sizeof events);
    memcpy(&other_ram, &ram, sizeof ram);
    assert(musashi_player_card_copy_completed_metadata(&player, &ram, &events, 2, id, NULL)
           == first_witness);
    assert(refused(&player, &ram, &events, 2, 0));
    assert(!memcmp(&before_player, &player, sizeof player));
    assert(!memcmp(&before_events, &events, sizeof events));
    assert(!memcmp(&other_ram, &ram, sizeof ram));
    assert(refused(&player, &ram, &events, 3, id));
    assert(refused(&player, &other_ram, &events, 2, id));
    assert(refused(&player, &ram, &other_events, 2, id));

    MusashiPlayerCard clone = player;
    assert(refused(&clone, &ram, &events, 2, id));
    MusashiPlayerCard other = {0};
    musashi_bios_events_init(&other_events);
    assert(musashi_player_card_open(&other, &other_ram, &other_events, NULL));
    assert(refused(&other, &other_ram, &other_events, 2, id));
    assert(refused(&other, &ram, &events, 2, id));
    musashi_player_card_close(&other);
    uint8_t *row = span(ROW_ADDR, CS_DIRENT_BYTES);
    row[24] ^= 1;
    assert(refused(&player, &ram, &events, 2, id));
    row[24] ^= 1;
    assert(copied(id, expected) == first_witness);
    set_path(alternate);
    assert(refused(&player, &ram, &events, 2, id));

    /* A different handle may read the same range without granting this file
       a completed read. Both handles run against the real mounted medium. */
    assert(cs_open(s, PATH_ADDR, 0x8001) == 3);
    set_path(name);
    assert(cs_seek(s, 3, 512, 0) == 512);
    assert(cs_read(s, 3, READ_ADDR, METADATA_SIZE) == 0);
    pump();
    assert(refused(&player, &ram, &events, 2, id));
    assert(cs_close(s, 3) == 3);
    assert(cs_close(s, 2) == 2);
    assert(refused(&player, &ram, &events, 2, id));

    uint64_t reopened = select_file(name);
    assert(reopened != id);
    submit(512, READ_ADDR, METADATA_SIZE);
    pump();
    assert(refused(&player, &ram, &events, 2, id));
    uint64_t first = copied(reopened, expected);
    assert(copied(reopened, expected) == first);

    /* Each completed read gets an identity even when all bytes are equal. */
    submit(512, READ_ADDR, METADATA_SIZE);
    assert(refused(&player, &ram, &events, 2, reopened));
    pump();
    assert(!memcmp(received, expected, METADATA_SIZE));
    uint64_t second = copied(reopened, expected);
    assert(second != first && copied(reopened, expected) == second);
    assert(cs_close(s, 2) == 2);
    uint64_t fresh = select_file(name);
    assert(fresh != reopened);
    submit(512, READ_ADDR, METADATA_SIZE);
    pump();
    assert(refused(&player, &ram, &events, 2, reopened));
    copied(fresh, expected);
    /* Move the host counter near exhaustion; continue exercising real reads. */
    player.metadata_read_id = UINT64_MAX - 1;
    submit(512, READ_ADDR, METADATA_SIZE);
    pump();
    assert(copied(fresh, expected) == UINT64_MAX);
    memset(received, 0xaa, METADATA_SIZE);
    submit(512, READ_ADDR, METADATA_SIZE);
    pump();
    assert(!memcmp(received, expected, METADATA_SIZE));
    assert(refused(&player, &ram, &events, 2, fresh));
    assert(cs_close(s, 2) == 2);
}

int main(int argc, char **argv)
{
    uint8_t expected[METADATA_SIZE] = {0};
    assert(argc == 4);
    if (!strcmp(argv[3], "pattern")) {
        for (size_t i = 0; i < sizeof expected; ++i) expected[i] = (uint8_t)(i * 13u + (i >> 4) + 7u);
    } else {
        assert(!strcmp(argv[3], "zeros"));
    }
    musashi_bios_events_init(&events);
    assert(musashi_player_card_open(&player, &ram, &events, argv[2]));
    assert(player.slot0_mounted);
    cs_init_card(player.service);
    cs_start_card(player.service);
    cs_init_files(player.service);
    if (!strcmp(argv[1], "create")) {
        seed_file(name, expected);
        seed_file(alternate, expected);
    } else {
        assert(!strcmp(argv[1], "verify"));
        verify(expected);
    }
    assert(musashi_player_card_finish(&player));
    musashi_player_card_close(&player);
    return 0;
}
