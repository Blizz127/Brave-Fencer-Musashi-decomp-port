#include "player_card.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* This fixture has no guest callback records or executable. */
int musashi_boot_execute_bios_event(void *unused, uint32_t callback)
{
    (void)unused;
    (void)callback;
    abort();
}

int main(int argc, char **argv)
{
    static MusashiBootMemory memory;
    MusashiPlayerCard card = {0};
    MusashiBiosEvents events;
    const uint32_t path = 64, data = 256;
    const size_t size = CS_SECTOR_BYTES;
    int32_t fd;
    assert(argc == 3);
    musashi_bios_events_init(&events);
    if (strcmp(argv[1], "existing-card-removed") == 0) {
        assert(musashi_player_card_open_selected(&card, &memory, &events, argv[2], 1));
        assert(!card.slot0_mounted);
        musashi_player_card_close(&card);
        return 0;
    }
    assert(musashi_player_card_open(&card, &memory, &events, argv[2]));
    cs_init_card(card.service);
    cs_start_card(card.service);
    cs_init_files(card.service);
    strcpy((char *)&memory.bytes[path], "bu00:CEDAR");

    if (strcmp(argv[1], "file-errors") == 0) {
        uint32_t error = 7, answer = 0;
        uint32_t args[4] = {path, 1, 0, 0};
        assert(musashi_player_card_vector(&card, 0xb0, 0x54, args, &answer, &error));
        assert(answer == 7);
        assert(musashi_player_card_vector(&card, 0xa0, 0, args, &answer, &error));
        assert(answer == UINT32_MAX && error == 2);
        args[1] = (1u << 16) | 0x0202u;
        assert(musashi_player_card_vector(&card, 0xa0, 0, args, &answer, &error));
        assert(answer >= 2 && answer < 16 && error == 2);
        args[0] = answer;
        assert(musashi_player_card_vector(&card, 0xb0, 0x55, args, &answer, &error));
        assert(answer == 0 && error == 2);
        /* Another file device may change the kernel's shared error. */
        error = 9;
        assert(musashi_player_card_vector(&card, 0xb0, 0x54, args, &answer, &error));
        assert(answer == 9);
        args[0] = path;
        args[1] = data;
        strcpy((char *)&memory.bytes[path], "bu00:NO-MATCH");
        assert(musashi_player_card_vector(&card, 0xb0, 0x42, args, &answer, &error));
        assert(answer == 0 && error == 9);
        strcpy((char *)&memory.bytes[path], "cdrom:FILE");
        assert(!musashi_player_card_vector(&card, 0xa0, 0, args, &answer, &error));
        assert(error == 9);
    } else if (strcmp(argv[1], "blank") == 0 || strcmp(argv[1], "format") == 0) {
        uint32_t args[4] = {path, 0, 0, 0}, answer = 0, error = 0;
        assert(cs_prepare(card.service, 0) == CS_MEDIA_UNFORMATTED);
        if (strcmp(argv[1], "format") == 0) {
            strcpy((char *)&memory.bytes[path], "bu00:");
            assert(musashi_player_card_vector(&card, 0xb0, 0x41, args, &answer, &error));
            assert(answer == 1 && cs_prepare(card.service, 0) == CS_MEDIA_OK);
        }
    } else if (strcmp(argv[1], "create") == 0) {
        fd = cs_open(card.service, path, (1u << 16) | 0x0202u);
        assert(fd >= 0);
        memset(&memory.bytes[data], 0x21, size);
        assert(cs_write(card.service, fd, data, size) == (int32_t)size);
        assert(cs_close(card.service, fd) == fd);
    } else if (strcmp(argv[1], "finish") == 0 || strcmp(argv[1], "cancel") == 0) {
        fd = cs_open(card.service, path, 0x8002u);
        assert(fd >= 0);
        memset(&memory.bytes[data], 0x73, size);
        assert(cs_write(card.service, fd, data, size) == 0);
        assert(!cs_idle(card.service));
        if (strcmp(argv[1], "finish") == 0) {
            assert(musashi_player_card_finish(&card));
            assert(cs_idle(card.service));
        }
    } else {
        int updated = strcmp(argv[1], "updated") == 0;
        assert(updated || strcmp(argv[1], "original") == 0);
        fd = cs_open(card.service, path, 1u);
        assert(fd >= 0);
        assert(cs_read(card.service, fd, data, size) == (int32_t)size);
        for (size_t i = 0; i < size; ++i)
            assert(memory.bytes[data + i] == (updated ? 0x73 : 0x21));
        assert(cs_close(card.service, fd) == fd);
    }
    musashi_player_card_close(&card);
    return 0;
}
