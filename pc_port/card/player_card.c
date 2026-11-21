#include "player_card.h"
#include "card_host_storage.h"
#include "musashi_sha256_digest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *map_guest(void *context, uint32_t address, size_t size, int writable)
{
    MusashiPlayerCard *card = context;
    uint32_t physical = address & 0x1fffffffu;
    (void)writable;
    if (!address || !size || (address >= 0x20000000u && address < 0x80000000u) ||
        address >= 0xc0000000u) return NULL;
    if (physical < MUSASHI_RAM_SIZE && size <= MUSASHI_RAM_SIZE - physical)
        return card->memory->bytes + physical;
    if (physical >= 0x1f800000u && physical < 0x1f800000u + MUSASHI_SCRATCHPAD_SIZE) {
        uint32_t offset = physical - 0x1f800000u;
        if (size <= MUSASHI_SCRATCHPAD_SIZE - offset)
            return card->memory->scratchpad + offset;
    }
    return NULL;
}

static CsMedia snapshot(void *context, unsigned slot, uint8_t *destination)
{
    MusashiPlayerCard *card = context;
    return cs_storage_snapshot(card->storage, slot, destination);
}

static int commit_image(void *context, unsigned slot, const uint8_t *expected,
                        const uint8_t *replacement)
{
    MusashiPlayerCard *card = context;
    return cs_storage_commit(card->storage, slot, expected, replacement);
}

static int deliver(void *context, uint32_t group, uint32_t spec)
{
    MusashiPlayerCard *card = context;
    /* Only replay authenticated leaves from the mounted US program. Delivery
     * can visit several records. Stage their guest writes and status changes
     * together so an unsupported later callback can retry the entire event. */
    static const uint8_t expected[32] = {
        0x49, 0xef, 0xee, 0xd0, 0x8c, 0x06, 0xa6, 0x6f, 0xd2, 0xc4, 0x9b, 0x47, 0x65, 0x2f, 0xc2, 0x26,
        0x73, 0x90, 0xa5, 0x2e, 0xbe, 0x73, 0x38, 0x89, 0xe3, 0xfd, 0x8b, 0x23, 0x5c, 0x73, 0xaf, 0x97
    };
    uint8_t digest[32];
    MusashiBiosEvents staged;
    if (musashi_bios_events_deliver_poll(card->events, group, spec)) return 1;
    const uint8_t *code = musashi_boot_ram_span(card->memory, 0x800617ccu, 160);
    if (!code || !musashi_sha256_digest(code, 160, digest) ||
        memcmp(digest, expected, sizeof digest)) return 0;
    if (!card->event_memory) card->event_memory = malloc(sizeof *card->event_memory);
    if (!card->event_memory) return 0;
    staged = *card->events;
    memcpy(card->event_memory, card->memory, sizeof *card->memory);
    if (!musashi_bios_events_deliver(&staged, group, spec,
                                    musashi_boot_execute_bios_event, card->event_memory)) return 0;
    for (size_t offset = 0; offset < MUSASHI_RAM_SIZE; offset += 4096) {
        uint8_t *live = card->memory->bytes + offset;
        const uint8_t *updated = card->event_memory->bytes + offset;
        if (memcmp(live, updated, 4096)) memcpy(live, updated, 4096);
    }
    memcpy(card->memory->scratchpad, card->event_memory->scratchpad, MUSASHI_SCRATCHPAD_SIZE);
    *card->events = staged;
    return 1;
}

int musashi_player_card_open(MusashiPlayerCard *card, MusashiBootMemory *memory,
                             MusashiBiosEvents *events, const char *directory)
{
    CsHost host = {card, map_guest, snapshot, commit_image, deliver};
    if (!card || !memory || !events || card->service || card->storage) return 0;
    card->memory = memory;
    card->events = events;
    card->storage = cs_storage_create();
    card->service = cs_create(host);
    if (!card->storage || !card->service) {
        musashi_player_card_close(card);
        return 0;
    }
    cs_vector_init(&card->vector, card->service, host);
    if (!directory) return 1;
    if (bfm_plat_memcard_set_dir(directory) != BFM_PLAT_OK ||
        cs_storage_attach(card->storage, 0) != BFM_PLAT_OK) {
        fputs("Memory card slot 1 unavailable: could not acquire its save directory.\n", stderr);
        return 1;
    }
    CsMedia media = cs_prepare(card->service, 0);
    if (media != CS_MEDIA_OK && media != CS_MEDIA_UNFORMATTED) {
        cs_media_changed(card->service, 0);
        cs_storage_detach(card->storage, 0);
        fputs("Memory card slot 1 unavailable: failed to load or create a valid image.\n", stderr);
    }
    return 1;
}

int musashi_player_card_finish(MusashiPlayerCard *card)
{
    if (!card || !card->service || cs_idle(card->service)) return 1;
    /* Each slot owns at most one command. A single pump visits both and
     * delivers available notifications without rerunning a completed write. */
    (void)cs_pump(card->service);
    return cs_idle(card->service);
}

void musashi_player_card_close(MusashiPlayerCard *card)
{
    if (!card) return;
    if (card->service) {
        if (!cs_idle(card->service))
            fputs("Memory card closed with a pending command or notification. Completed writes remain on disk.\n", stderr);
        cs_media_changed(card->service, 0);
        cs_destroy(card->service);
    }
    if (card->storage) cs_storage_destroy(card->storage);
    free(card->event_memory);
    *card = (MusashiPlayerCard){0};
}

void musashi_player_card_pulse(MusashiPlayerCard *card, uint64_t source_vblank)
{
    if (!card || !card->service || card->last_vblank == source_vblank) return;
    card->last_vblank = source_vblank;
    if (!cs_idle(card->service)) (void)cs_pump(card->service);
}

int musashi_player_card_vector(MusashiPlayerCard *card, uint32_t vector, uint32_t selector,
                               const uint32_t args[4], uint32_t *v0, uint32_t *file_error)
{
    CsVectorResult result;
    if (!args || !v0 || !file_error) return 0;
    if (vector == 0xb0 && selector == 0x54) {
        *v0 = *file_error;
        return 1;
    }
    if (!card || !card->service) return 0;
    /* Initialization also installs the port's existing controller and BIOS
     * state. Its owner updates the file service after accepting each call. */
    if ((vector == 0xb0 && (selector == 0x4a || selector == 0x4b)) ||
        (vector == 0xa0 && selector == 0x70)) return 0;
    result = cs_vector_dispatch(&card->vector, vector, selector,
                                args[0], args[1], args[2], args[3]);
    if (!result.recognized) return 0;
    if (result.file_error) *file_error = (uint32_t)result.file_error;
    if (result.write_v0) *v0 = result.v0;
    return 1;
}
