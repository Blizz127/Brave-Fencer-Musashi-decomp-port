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
#define PAYLOAD_ADDR UINT32_C(0x80075cc0)
enum { METADATA_SIZE = 640, PAYLOAD_SIZE = 768 };
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

/* The guest reads a pointer and byte length here, then walks 80-byte drivers. */
#define DEVICE_TABLE UINT32_C(0x80000150)
enum { DEVICE_BYTES = 80, DIRECTORY_CALLBACK_OFFSET = 52 };

static uint32_t device_word(uint32_t address)
{
    uint32_t value;
    assert(musashi_boot_read32(&ram, address, &value));
    return value;
}

static uint32_t cached_ram(uint32_t address)
{
    assert(address < MUSASHI_RAM_SIZE ||
           (address >= 0x80000000u && address < 0x80000000u + MUSASHI_RAM_SIZE) ||
           (address >= 0xa0000000u && address < 0xa0000000u + MUSASHI_RAM_SIZE));
    return 0x80000000u | (address & (MUSASHI_RAM_SIZE - 1u));
}

static void refused_init_preserves_ram(void)
{
    memcpy(&other_ram, &ram, sizeof ram);
    assert(!musashi_player_card_init_files(&player));
    assert(!memcmp(&other_ram, &ram, sizeof ram));
}

static void install_file_driver(uint32_t *record, uint32_t *device_name)
{
    assert(!device_word(DEVICE_TABLE) && !device_word(DEVICE_TABLE + 4));
    refused_init_preserves_ram();
    cs_init_card(player.service);
    refused_init_preserves_ram();
    cs_start_card(player.service);
    assert(musashi_player_card_init_files(&player));
    assert(device_word(DEVICE_TABLE) && device_word(DEVICE_TABLE + 4) == DEVICE_BYTES);
    *record = cached_ram(device_word(DEVICE_TABLE));
    assert(device_word(*record));
    *device_name = cached_ram(device_word(*record));
    assert(!memcmp(span(*device_name, 3), "bu", 3));

    /* The guest can replace this driver callback while enumerating a directory. */
    const uint32_t callback = UINT32_C(0x80010000);
    assert(musashi_boot_write32(&ram, *record + DIRECTORY_CALLBACK_OFFSET, callback));
    memcpy(&other_ram, &ram, sizeof ram);
    assert(musashi_player_card_init_files(&player));
    assert(!memcmp(&other_ram, &ram, sizeof ram));
}

static void check_registration_conflicts(const char *directory, uint32_t record, uint32_t device_name)
{
    memset(&ram, 0, sizeof ram);
    musashi_bios_events_init(&events);
    assert(musashi_player_card_open_selected(&player, &ram, &events, directory, 1));
    assert(player.slot0_mounted);
    cs_init_card(player.service);
    cs_start_card(player.service);

    assert(musashi_boot_write32(&ram, DEVICE_TABLE, OTHER_ADDR));
    assert(musashi_boot_write32(&ram, DEVICE_TABLE + 4, DEVICE_BYTES));
    *span(OTHER_ADDR, 1) = 0x72;
    refused_init_preserves_ram();
    assert(musashi_boot_write32(&ram, DEVICE_TABLE, 0));
    refused_init_preserves_ram();
    assert(musashi_boot_write32(&ram, DEVICE_TABLE + 4, 0));

    *span(record + DEVICE_BYTES - 1, 1) = 0x37;
    refused_init_preserves_ram();
    *span(record + DEVICE_BYTES - 1, 1) = 0;
    *span(device_name, 1) = 'x';
    refused_init_preserves_ram();
    *span(device_name, 1) = 0;
    assert(musashi_player_card_init_files(&player));

    /* Another driver took the header. Closing may not free its guest RAM. */
    assert(musashi_boot_write32(&ram, DEVICE_TABLE, OTHER_ADDR));
    refused_init_preserves_ram();
    musashi_player_card_close(&player);
    assert(!memcmp(&other_ram, &ram, sizeof ram));
}

static void check_file_vectors(void)
{
    uint32_t result = 0, error = 0;
    uint32_t args[4] = {PATH_ADDR, ROW_ADDR, 0, 0};
    static const char missing[] = "bu00:ABSENT";
    memcpy(span(PATH_ADDR, sizeof missing), missing, sizeof missing);
    assert(musashi_player_card_vector(&player, 0xb0, 0x42, args, &result, &error));
    assert(result == 0);
    args[1] = 1;
    assert(musashi_player_card_vector(&player, 0xb0, 0x32, args, &result, &error));
    assert(result == UINT32_MAX && error == 2);

    memcpy(span(PATH_ADDR, sizeof name), name, sizeof name);
    args[1] = ROW_ADDR;
    assert(musashi_player_card_vector(&player, 0xb0, 0x42, args, &result, &error));
    assert(result == ROW_ADDR && !memcmp(span(ROW_ADDR, 20), name + 5, 20));
    args[0] = ROW_ADDR;
    assert(musashi_player_card_vector(&player, 0xb0, 0x43, args, &result, &error));
    assert(!result);
}

static void set_path(const char *full_name)
{
    assert(strlen(full_name) == 24);
    memcpy(span(PATH_ADDR, 25), full_name, 25);
}

static void seed_file(const char *full_name, const uint8_t bytes[METADATA_SIZE],
                      const uint8_t payload[4][PAYLOAD_SIZE])
{
    set_path(full_name);
    assert(cs_open(player.service, PATH_ADDR, (1u << 16) | 0x203) == 2);
    assert(cs_seek(player.service, 2, 512, 0) == 512);
    memcpy(span(OTHER_ADDR, METADATA_SIZE), bytes, METADATA_SIZE);
    assert(cs_write(player.service, 2, OTHER_ADDR, METADATA_SIZE) == METADATA_SIZE);
    for (unsigned row = 0; row < 4; ++row) {
        memcpy(span(OTHER_ADDR, PAYLOAD_SIZE), payload[row], PAYLOAD_SIZE);
        assert(cs_write(player.service, 2, OTHER_ADDR, PAYLOAD_SIZE) == PAYLOAD_SIZE);
    }
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

static uint64_t payload_id(int fd, unsigned file, uint32_t position, uint32_t count)
{
    return musashi_player_card_payload_descriptor(&player, &ram, &events, fd, file, position, count);
}

static void verify_payload(unsigned file, const uint8_t expected[PAYLOAD_SIZE])
{
    static const uint32_t offsets[] = {1152u, 1920u, 2688u, 3456u};
    CardService *s = player.service;
    uint32_t offset = offsets[file - 1], end = offset + PAYLOAD_SIZE;
    set_path(name);
    assert(cs_first(s, PATH_ADDR, ROW_ADDR) == ROW_ADDR);
    uint64_t previous = 0;
    for (unsigned occupied = 0; occupied < 2; ++occupied) {
        int other = -1;
        if (occupied) {
            set_path(alternate);
            other = cs_open(s, PATH_ADDR, 0x8001);
            assert(other >= 0);
            set_path(name);
            assert(!payload_id(other, file, 0, 0));
        }
        int fd = cs_open(s, PATH_ADDR, 0x8001);
        assert(fd >= 0 && fd != other);
        uint64_t id = payload_id(fd, file, 0, 0);
        assert(id && id != previous);
        assert(!payload_id(fd, 0, 0, 0) && !payload_id(fd, 5, 0, 0));
        previous = id;
        assert(cs_seek(s, fd, end, 0) == (int32_t)end && !payload_id(fd, file, end, 0));
        assert(cs_seek(s, fd, offset, 0) == (int32_t)offset);
        assert(!payload_id(fd, file, offset, METADATA_SIZE));
        assert(payload_id(fd, file, offset, PAYLOAD_SIZE) == id);
        assert(cs_read(s, fd, OTHER_ADDR, PAYLOAD_SIZE) == 0);
        pump();
        assert(!payload_id(fd, file, end, 0));
        assert(cs_seek(s, fd, offset, 0) == (int32_t)offset);
        uint8_t *destination = span(PAYLOAD_ADDR, PAYLOAD_SIZE);
        memset(destination, 0xe6, PAYLOAD_SIZE);
        assert(cs_read(s, fd, PAYLOAD_ADDR, PAYLOAD_SIZE) == 0);
        assert(!payload_id(fd, file, end, 0) && destination[0] == 0xe6);
        pump();
        assert(!memcmp(destination, expected, PAYLOAD_SIZE) && payload_id(fd, file, end, 0) == id);
        for (unsigned wrong = 1; wrong <= 4; ++wrong) {
            if (wrong == file) continue;
            assert(!payload_id(fd, wrong, end, 0));
            assert(!payload_id(fd, wrong, offsets[wrong - 1] + PAYLOAD_SIZE, 0));
        }
        destination[PAYLOAD_SIZE - 1] ^= 1;
        assert(!payload_id(fd, file, end, 0));
        destination[PAYLOAD_SIZE - 1] ^= 1;
        assert(payload_id(fd, file, end, 0) == id);
        if (file == 4 && occupied) {
            cs_media_changed(s, 0);
            assert(!payload_id(fd, file, end, 0));
        }
        assert(cs_close(s, fd) == fd && !payload_id(fd, file, end, 0));
        if (other >= 0) assert(cs_close(s, other) == other);
    }
}

int main(int argc, char **argv)
{
    uint8_t expected[METADATA_SIZE] = {0};
    uint32_t record, device_name;
    assert(argc == 4);
    if (!strcmp(argv[3], "pattern")) {
        for (size_t i = 0; i < sizeof expected; ++i) expected[i] = (uint8_t)(i * 13u + (i >> 4) + 7u);
    } else {
        assert(!strcmp(argv[3], "zeros"));
    }
    uint8_t payload[4][PAYLOAD_SIZE] = {{0}};
    if (!strcmp(argv[3], "pattern")) {
        for (unsigned row = 0; row < 4; ++row)
            for (size_t i = 0; i < PAYLOAD_SIZE; ++i)
                payload[row][i] = (uint8_t)(expected[i % METADATA_SIZE] + 41u * row);
    }
    musashi_bios_events_init(&events);
    assert(musashi_player_card_open(&player, &ram, &events, argv[2]));
    assert(player.slot0_mounted);
    install_file_driver(&record, &device_name);
    if (!strcmp(argv[1], "create")) {
        seed_file(name, expected, payload);
        seed_file(alternate, expected, payload);
    } else {
        assert(!strcmp(argv[1], "verify"));
        check_file_vectors();
        verify(expected);
        for (unsigned file = 1; file <= 4; ++file) verify_payload(file, payload[file - 1]);
    }
    assert(musashi_player_card_finish(&player));
    musashi_player_card_close(&player);
    assert(!device_word(DEVICE_TABLE) && !device_word(DEVICE_TABLE + 4));
    const uint8_t empty_record[DEVICE_BYTES] = {0};
    assert(!memcmp(span(record, DEVICE_BYTES), empty_record, DEVICE_BYTES));
    assert(!memcmp(span(device_name, 3), empty_record, 3));
    if (!strcmp(argv[1], "verify")) check_registration_conflicts(argv[2], record, device_name);
    return 0;
}
